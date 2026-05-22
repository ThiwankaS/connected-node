#include "include/app_main.hpp"

extern "C" {
    #include "driver/gpio.h"
    #include "esp_log.h"
    #include "freertos/idf_additions.h"
    #include "freertos/projdefs.h"
    #include "hal/gpio_types.h"
    #include "soc/gpio_num.h"
}

namespace loop_polling {
    /**
     * @class ToggleLED
     * @brief Manages the hardware configuration, edge-detection logic, and execution lifecycle of the 
     * LED toggling system.
     */
    class ToggleLED {
        public:
            /**
             * @fn run
             * @brief Coordinates application startup, initializes hardware peripherals, and executes the continuous polling processing loop.
             */
            void run();
        private:
            /**
             * @brief Defines the compile-time constant for the onboard hardware LED pin identifier
             * and BOOT button pin identifier
             */
            static constexpr gpio_num_t kLed = GPIO_NUM_2;
            static constexpr gpio_num_t kButton = GPIO_NUM_0;
            /**
             * @brief Defines the compile-time constant logging tag used for identification in serial 
             * diagnostics.
             */
            static constexpr const char* TAG = "APP";

            /**
             * @fn configure
             * @brief Configures internal hardware parameters, directional operational modes, 
             * and register maps for GPIO peripherals.
             */
            void configure();

    };

    /**
     * @fn configure
     * @brief Allocates parameters, defines bitmasks, and applies structural registers 
     * to the physical microcontroller pins.
     */
    void ToggleLED::configure() {
        
        /**
         * @struct led
         * @brief Configuration structure instance used to define the digital parameters of the LED pin.
         */
        gpio_config_t led {};
        led.pin_bit_mask = 1ULL << kLed;
        led.mode = GPIO_MODE_OUTPUT;
        gpio_config(&led);

        /**
         * @struct button
         * @brief Configuration structure instance used to define the digital parameters of the button pin.
         */
        gpio_config_t button{};
        button.pin_bit_mask = 1ULL << kButton;
        button.mode = GPIO_MODE_INPUT;
        gpio_config(&button);
    }

    /**
     * @fn run
     * @brief Executes the runtime lifecycle, monitors peripheral transitions, 
     * and manages OS thread scheduling states.
     */
    void ToggleLED::run() {

        configure();
        ESP_LOGI(TAG, "LED GPIO : %d - BOOT GPIO : %d", kLed, kButton);
        
        /**
         * @var level
         * @brief Tracks the persistent active logical output level applied 
         * to the physical LED peripheral pin.
         */
        int level = 0;
        /**
         * @var last_btn
         * @brief Stores the historical logic state of the button read during 
         * the preceding execution loop cycle.
         */
        int last_btn = 1;

        /**
         * @loop infinite_polling_loop
         * @brief Drives continuous hardware sampling, edge validation, software debouncing,
         * and RTOS priority yielding.
         */
        for(;;) {
            
            /**
             * @var btn
             * @brief Samples the current real-time digital logic level of the physical button peripheral pin*/
            const int btn = gpio_get_level(kButton);
            
            /**
             * @cond falling_edge_check
             * @brief Evaluates whether the button state has transitioned from an unpressed logic high
             * to a pressed logic low.
             */
            if(last_btn == 1 && btn == 0) {
                level = !level;
                gpio_set_level(kLed, level);
                ESP_LOGI(TAG, "LED %s", level ? "ON" : "OFF");
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            last_btn = btn;
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
} // namespace loop_polling

ESP32_APP_MAIN(loop_polling::ToggleLED);
