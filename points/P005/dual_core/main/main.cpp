#include <cstdint>

#include "include/app_main.hpp"
#include "include/custome_task.hpp"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"

namespace custome {
    /**
     * @class DualTask
     * @brief Manages two concurrent tasks: an LED toggler and a system status logger.
     */
    class DualTask {
        public:
            /**
             * @brief Initializes and kicks off both concurrent loops as FreeRTOS tasks.
             */
            void run();
        private:
            static constexpr gpio_num_t kLed = GPIO_NUM_2; // GPIO pin connected to the onboard LED.
            static constexpr const char* TAG = "DUAL_APP"; // Logging tag used for ESP_LOG outputs.

            /**
             * @brief Infinite loop that toggles the status of the designated LED.
             * @note Runs at a 500ms interval (1Hz toggle rate).
             */
            void led_loop();
            /**
             * @brief Infinite loop that gathers and logs system statistics.
             * @note Prints system uptime and available heap memory every 2000ms.
             */
            void stat_loop();
    };

    /**
     * @brief Toggles the GPIO LED state repeatedly.
     * * Configures the GPIO pin direction to output and enters an infinite loop,
     * inverting the LED state every 500 milliseconds.
     */
    void DualTask::led_loop() {
        gpio_set_direction( kLed ,GPIO_MODE_OUTPUT);
        int level = 0;
        for(;;) {
            gpio_set_level(kLed, level);
            level = !level;
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    /**
     * @brief Logs uptime and free heap size over the serial monitor.
     * * Periodically samples the ESP hardware timer and heap allocator API, 
     * printing formatted system diagnostics every 2 seconds.
     */
    void DualTask::stat_loop() {
        for(;;) {
            const int64_t us = esp_timer_get_time();
            ESP_LOGI(TAG, "uptime = %lld s | heap = %u",
                    static_cast<long long>(us / 1000000),
                    static_cast<unsigned long>(esp_get_free_heap_size()));
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    /**
     * @brief Launches the LED and Stats tasks under the custom task wrapper.
     * * Spawns `led_loop` with a 2KB stack (Priority 2) and `stat_loop` with a 
     * 4KB stack (Priority 1), passing the current instance (`this`) pointer context.
     */
    void DualTask::run() {
        custome::Task::start<DualTask, &DualTask::led_loop>("led", 2048, 2, this);
        custome::Task::start<DualTask, &DualTask::stat_loop>("stat", 4096, 1, this);
        ESP_LOGI(TAG, "C++ member task runnig");
    }
}
/**
 * @brief Application entry point macro initializing the DualTask lifecycle.
 */
ESP32_APP_MAIN(custome::DualTask);
