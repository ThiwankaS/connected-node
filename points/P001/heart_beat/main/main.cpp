#include <cstdint>
#include <cinttypes>

#include "include/app_main.hpp"
#include "include/esp_idf_c.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "sdkconfig.h"

extern "C" {
    #include "esp_chip_info.h"
    #include "esp_flash.h"
}

namespace heart_beat {
    /**
      * @class ToolchainApp
      * @brief Handles application hardware diagnostics and system validation lifecycle.
      */
    class ToolChainApp {
        public:
            /**
              * @brief Core execution context invoked by the entrypoint macro initialization wrapper.
              */
            void run();
        private:
            /* Compile-time constant identifier used for filtered log outputs */
            static constexpr const char* TAG = "APP";
    };

    void ToolChainApp::run() {
        /* Initialize an empty hardware information struct to avoid garbage data */
        esp_chip_info_t chip_info {};
        esp_chip_info(&chip_info);

        /* Retrieve the onboard/external primary SPI flash configuration metrics */
        uint32_t flash_bytes = 0;
        esp_flash_get_size(nullptr, &flash_bytes);

        /* Convert the capacity representation from raw bytes to Megabytes (MB) */
        const uint32_t flash_mb = flash_bytes / (1024U * 1024U);

        /* Log environment state and hardware topology layout matching configuration inputs */
        ESP_LOGI(TAG, "ESP Programming with C++ - OK");
        ESP_LOGI(TAG, 
                "Target : %s | cores : %d | rev : %d | flash : %" PRIu32 "MB", 
                CONFIG_IDF_TARGET, 
                chip_info.cores, 
                chip_info.revision,
                flash_mb);

        /* Periodic verification loop monitoring continuous operation stability and resource leakages */
        for(int i = 0;;++i) {
            ESP_LOGI(TAG, "heart beat : %d | free heap : %" PRIu32,
                    i,
                    static_cast<uint32_t>(esp_get_free_heap_size()));
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
} // namespace heart_beat

/* Hook the newly declared C++ class context directly into the underlying system boot sequence */
ESP32_APP_MAIN(heart_beat::ToolChainApp);
