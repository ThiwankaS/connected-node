/**
 * @file main.cpp
 * @brief UART console command handler for on-board LED control (ESP32).
 *
 * Architecture overview:
 * - USB-UART (GPIO1 TX / GPIO3 RX) is wired to UART0 and registered by ESP-IDF
 *   as the system console (stdout/stderr/stdin via VFS).
 * - `idf.py monitor` sends keystrokes to the chip; the firmware must echo
 *   characters back because the monitor does not provide local echo.
 * - Input is handled in a dedicated FreeRTOS task using non-blocking `read()`
 *   on STDIN. This avoids `select()` on VFS (which can starve IDLE and trip TWDT)
 *   and keeps the `main` task free to complete after startup.
 */

#include <cerrno>
#include <cstring>
#include "include/app_main.hpp"

#include "hal/gpio_types.h"
#include "soc/gpio_num.h"

#include <fcntl.h>
#include <unistd.h>

extern "C" {
    #include "driver/gpio.h"
    #include "esp_log.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

namespace uart {

/**
 * @class Console
 * @brief Interactive serial console that parses text commands and drives an LED.
 *
 * Commands are received as line-oriented text over the ESP-IDF console UART.
 * Each complete line (terminated by CR or LF, as sent by idf.py monitor) is
 * compared against fixed command strings and mapped to GPIO actions or help text.
 */
class Console {
    public:
        /**
         * @brief Performs one-time hardware setup and starts the console worker task.
         *
         * Configures the LED GPIO, prints the command list, shows the initial prompt,
         * then creates the `"console"` FreeRTOS task. This function returns to
         * `app_main` immediately so the startup task does not run the input loop.
         */
        void run();

    private:
        /** @brief On-board LED pin (typical ESP32 DevKit built-in LED on GPIO2). */
        static constexpr gpio_num_t kLed = GPIO_NUM_2;

        /** @brief Tag used by ESP_LOGx macros for filtered log output. */
        static constexpr const char* TAG = "APP";

        /**
         * @brief Sets the LED GPIO level and logs the new state.
         * @param on true drives the pin high (LED on), false drives it low (LED off).
         */
        void set_led(bool on);

        /**
         * @brief Writes the interactive command prompt to the serial console.
         *
         * Uses STDOUT_FILENO so output goes through the same VFS UART path as
         * ESP_LOGx, keeping monitor and firmware on one serial stream.
         */
        void write_prompt();

        /**
         * @brief Blocking worker loop: read UART stdin, echo keys, dispatch commands.
         *
         * Runs forever in the `"console"` task. Uses non-blocking stdin so the task
         * can call vTaskDelay and yield CPU time to IDLE (TWDT-safe).
         */
        void task_loop();

        /**
         * @brief FreeRTOS task trampoline required by xTaskCreate (C callback).
         * @param arg Pointer to the Console instance (`this` from run()).
         */
        static void task_entry(void* arg);
};

void Console::set_led(bool on) {
    gpio_set_level(kLed, on ? 1 : 0);
    ESP_LOGI(TAG, "LED is %s", on ? "ON" : "OFF");
}

void Console::write_prompt() {
    static constexpr char kPrompt[] = "\r\n> ";
    write(STDOUT_FILENO, kPrompt, sizeof(kPrompt) - 1);
}

void Console::task_loop() {
    /**
     * Put stdin into non-blocking mode. Without this, read() could block inside the
     * UART VFS driver and prevent vTaskDelay from running, starving IDLE0 on CPU0.
     */
    const int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    /** Accumulated command text until Enter is pressed. */
    char line[64];
    int line_len = 0;

    /** Scratch buffer for one read() syscall from stdin. */
    char buffer[256];

    for (;;) {
        const int len = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

        if (len < 0) {
            /* No data yet (EAGAIN/EWOULDBLOCK) or a real I/O error. */
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                ESP_LOGW(TAG, "stdin read error: %d", errno);
            }
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if (len == 0) {
            /* VFS may return 0 when the port is idle; back off before polling again. */
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        /* Process every byte from this read chunk (may be partial line or many keys). */
        for (int i = 0; i < len; ++i) {
            const char c = buffer[i];

            if (c == '\r' || c == '\n') {
                /* Line complete: idf monitor default EOL for ESP32 is CR. */
                write(STDOUT_FILENO, "\r\n", 2);

                if (line_len > 0) {
                    line[line_len] = '\0';

                    if (std::strcmp(line, "LED ON") == 0) {
                        set_led(true);
                    } else if (std::strcmp(line, "LED OFF") == 0) {
                        set_led(false);
                    } else if (std::strcmp(line, "HELP") == 0) {
                        static constexpr char kHelp[] =
                            "Commands : LED ON | LED OFF | HELP";
                        write(STDOUT_FILENO, kHelp, sizeof(kHelp) - 1);
                    } else {
                        ESP_LOGW(TAG, "Unknown command: %s", line);
                    }
                    line_len = 0;
                }

                write_prompt();
            } else if (c == '\b' || c == 127) {
                /* Backspace (ASCII 8) or DEL (127): edit line and fix terminal display. */
                if (line_len > 0) {
                    --line_len;
                    static constexpr char kBackspace[] = "\b \b";
                    write(STDOUT_FILENO, kBackspace, sizeof(kBackspace) - 1);
                }
            } else if (line_len < static_cast<int>(sizeof(line) - 1)) {
                /* Echo character so idf.py monitor shows typing (no local echo). */
                line[line_len++] = c;
                write(STDOUT_FILENO, &c, 1);
            }
        }

        /* Short yield after handling data so other tasks (including IDLE) run. */
        vTaskDelay(1);
    }
}

void Console::task_entry(void* arg) {
    static_cast<Console*>(arg)->task_loop();
}

void Console::run() {
    gpio_config_t led_cfg {};
    led_cfg.pin_bit_mask = 1ULL << kLed;
    led_cfg.mode = GPIO_MODE_OUTPUT;
    gpio_config(&led_cfg);
    set_led(false);

    ESP_LOGI(TAG, "Commands : LED ON | LED OFF | HELP");
    write_prompt();

    /** Stack size in words; console loop uses modest local buffers only. */
    constexpr uint32_t kStackWords = 4096;

    /**
     * Priority 5 is above idle but below time-critical drivers. The task owns
     * all stdin polling so `main` can exit `run()` without blocking.
     */
    BaseType_t created = xTaskCreate(
        task_entry, "console", kStackWords, this, 5, nullptr);

    if (created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create console task");
    }
}

} // namespace uart

/**
 * Register uart::Console as the firmware application entry point.
 * See app_main.hpp for how ESP32_APP_MAIN connects to ESP-IDF boot flow.
 */
ESP32_APP_MAIN(uart::Console);
