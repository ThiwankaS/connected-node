/**
  * @file main.cpp
  * @brief Implmentaion of a FreeRTOS Producer-Consumer pattren using ESP32 ADC
  * This program demostrate how to read data from an anlog sensor using ADC oneshot driver
  * pack into a custome C++ struct, pass it through a FreeRTOS queue, and safely consume
  * inside it inside an independent low priority task
*/

#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/adc_types.h"
#include "include/app_main.hpp"
#include "include/custome_task.hpp"
#include "portmacro.h"

extern "C" {
    #include "esp_log.h"
    #include "esp_adc/adc_oneshot.h"
}

/**
  * @namespace pass_data
  * @brief Namespace container for data-passing application struture and logic
*/
namespace pass_data {

    /**
      * @struct SensorData
      * @ brief A simple structure holding two integer values
    */
    struct SensorData {
        int raw;    // digital reading from the ADC (0 - 4095)
        int mv;     // calculated voltage approximation value to milivolt value
    };

    /**
      * @class QueueApp
      * @breif Handles hardware initialization, manages the FreeRTOS commiunication queue
      * and define the logic for the task
    */
    class QueueApp {
        public:
            /**
              * @function run
              * @brief Harware driver set-up routine and application entry point
            */
            void run();
        private:
            static constexpr const char* TAG = "PASS_DATA"; // Diagnostice logging identifier tag
            QueueHandle_t queue { nullptr };                // FreeRTOS handle managing the commiunications queue therad-safely
            adc_oneshot_unit_handle_t adc { nullptr };      // Handle the intializing ADC unit instance

            /**
              * @function producer_loop
              * @breif High-priority execution loop resposible for hardware sampling and populating the queue
            */
            void producer_loop();
            /**
              * @function consumer_loop
              * @brief Low-priority execution loop that process data packets as they arrived
            */
            void consumer_loop();
    };

    /**
     * @function producer_loop
     * @brief Continuously samples the ADC channel and pushes records to the queue.
     */
    void QueueApp::producer_loop() {
        for(;;) {
            int raw = 0;
            // Capture a raw digital sample from the configured ADC unit on Channel 0
            adc_oneshot_read(adc, ADC_CHANNEL_0, &raw);
            // Map the raw digital count directly to millivolts using a 12-bit linear formula (3.3V scale)
            const SensorData sample {
                .raw = raw,
                .mv = (raw * 3300) / 4095
            };
            // Non-blocking attempt to push the structural data onto the back of the queue
            // If the queue remains full for 100ms, abort and throw a warning log statement
            if(xQueueSend(queue, &sample, pdMS_TO_TICKS(100)) != pdTRUE) {
                ESP_LOGW(TAG, "Queue full!");
            }
            // Yield execution to the scheduler for 200ms before reading the sensor again
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    /**
     * @fucntion consumer_loop
     * @brief Blockingly consumes data packets pushed to the shared communications queue.
     */
    void QueueApp::consumer_loop() {
        
        SensorData sample {}; // Local cache structure instance to unpack queue elements into

        for(;;) {
            // Block indefinitely (portMAX_DELAY) until a data item is available to pop out of the queue
            if(xQueueReceive(queue, &sample, portMAX_DELAY) == pdTRUE) {
                // Safely log the received values down the system's serial monitor channel
                ESP_LOGI(TAG, "Sample raw = %d | mv = %d", sample.raw, sample.mv);
            }
        }
    }

    /**
     * @function run
     * @brief Allocates heap resources, hooks hardware layers, and launches runtime threads.
     */
    void QueueApp::run() {

        // Initialize the configuration profile for ADC Unit 1
        adc_oneshot_unit_init_cfg_t init_cfg {};
        init_cfg.unit_id = ADC_UNIT_1;
        // Construct and save the operational driver instance context wrapper
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc));

        // Fine-tune runtime specifications for Channel 0 (resolution and signal input matching)
        adc_oneshot_chan_cfg_t chan_cfg {};
        chan_cfg.bitwidth = ADC_BITWIDTH_12;    // Configure the unit to process sampling at 12-bit precision
        chan_cfg.atten = ADC_ATTEN_DB_12;       // Apply internal attenuation padding up to ~3.3V readable bounds
        // Bind the detailed channel configuration properties directly into our physical unit runtime profile
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc, ADC_CHANNEL_0, &chan_cfg));
        // Allocate heap space memory blocks managing up to 8 serialized SensorData structural nodes
        queue = xQueueCreate(8, sizeof(SensorData));
        
        /**
         * Provision and launch asynchronous background loops through custom template wrapper calls
         */
        // High-priority producer loop gets 4KB of stack space allocation at Priority level 2
        custome::Task::start<QueueApp, &QueueApp::producer_loop>("producer", 4096, 2, this);
        // Low-priority consumer loop gets 4KB of stack space allocation at Priority level 1
        custome::Task::start<QueueApp, &QueueApp::consumer_loop>("consumer", 4096, 1, this);
    }

} // namespace pass_data

ESP32_APP_MAIN(pass_data::QueueApp);
