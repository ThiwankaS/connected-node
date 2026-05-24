#include <cstdint>

#include "hal/adc_types.h"
#include "include/app_main.hpp"
#include "soc/clk_tree_defs.h"

extern "C" {
    #include "esp_log.h"
    #include "esp_err.h"
    #include "esp_system.h"

    #include "esp_adc/adc_cali.h"
    #include "esp_adc/adc_cali_scheme.h"
    #include "esp_adc/adc_oneshot.h"

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
    #include "freertos/semphr.h"
    #include "freertos/event_groups.h"
}

namespace analog {

    /**
     * @class Convertor
     * @brief Manages the initialization, line-fitting calibration, and sampling lifecycle 
     * of an Analog-to-Digital Converter (ADC) peripheral using the ESP32 oneshot driver.
     */
    class Convertor {
        public:
            /**
             * @brief Executes the primary application execution sequence, configuring 
             * hardware blocks and establishing a continuous sampling loop.
             */
            void run();
        private:
             static constexpr adc_unit_t kUnit = ADC_UNIT_1; // Target ADC unit hardware block instance
             static constexpr adc_channel_t kChannel = ADC_CHANNEL_0; // Designated hardware input channel
             static constexpr adc_atten_t kAttenuation = ADC_ATTEN_DB_12; // Attenuation level defining voltage input range limits
             static constexpr adc_bitwidth_t kBitwidht = ADC_BITWIDTH_12; // Precision bit-width configuration for digitized samples
             static constexpr const char* TAG = "ADC_APP"; // Logging tag identifier used for diagnostic system outputs

             adc_oneshot_unit_handle_t adc_ { nullptr }; // Hardware descriptor handle for the initialized ADC unit instance
             adc_cali_handle_t cali_ { nullptr }; // Runtime handle managing the registered calibration scheme properties
             bool calibration_ok = false; // State flag tracking whether valid calibration logic is active

             /**
             * @brief Initializes and registers a line-fitting calibration scheme for the 
             * active configuration.
             * @return true if calibration handle registration returns ESP_OK, false otherwise.
             */
             bool init_calibration();
    };

    /**
     * @brief Allocates line-fitting configurations and creates an ADC calibration context.
     * @return True upon successful initialization; logs a warning and returns false if instantiation fails.
     */
    bool Convertor::init_calibration() {
        esp_err_t ret = ESP_FAIL;

        adc_cali_line_fitting_config_t cfg {
            .unit_id = kUnit,
            .atten = kAttenuation,
            .bitwidth = kBitwidht,
            .default_vref = 1100
        };
        ret = adc_cali_create_scheme_line_fitting(&cfg, &cali_);

        if(ret == ESP_OK) {
            return true;
        }

        ESP_LOGW(TAG, "ADC calibration not availabe (%s) ", esp_err_to_name(ret));
        return false;
    }

    /**
     * @brief Instantiates the ADC oneshot unit driver, links designated pin channels,
     * validates calibration status, and loops continuously to poll sensor inputs.
     */
    void Convertor::run() {
        // Allocate and assign configuration parameters for the generic hardware ADC block
        adc_oneshot_unit_init_cfg_t init_cfg {};
        init_cfg.unit_id = kUnit;
        init_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_));

        // Define dynamic parameters, including attenuation values and bits of resolution
        adc_oneshot_chan_cfg_t chan_cfg {};
        chan_cfg.atten = kAttenuation;
        chan_cfg.bitwidth = kBitwidht;
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_, kChannel, &chan_cfg));

        // Evaluate the baseline structural response accuracy using reference voltages
        calibration_ok = init_calibration();
        ESP_LOGI(TAG, "ADC1 CH0 - calibraion is : %s", calibration_ok ? "ON" : "OFF");

        // Main peripheral monitoring tracking pipeline
        for(;;) {
            
            int raw = 0;
            ESP_ERROR_CHECK(adc_oneshot_read(adc_, kChannel, &raw));
            
            // Apply the calculated lookup slope factors to parse millivolts from raw integer measurements
            int mv = 0;
            if(calibration_ok) {
                ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_, raw, &mv));
            }

            ESP_LOGI(TAG, "raw : %d | mv : %d ", raw, mv);
            // Suspend task execution flow for 500 milliseconds to manage system scheduling
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

} // namespace analog

ESP32_APP_MAIN(analog::Convertor);;
