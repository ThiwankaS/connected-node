#pragma once

/**
 * @file esp_idf_c.h
 * @brief Global system and operating system includes for the application.
 */

/* Prevent name mangling when compiled with a C++ toolchain */
#ifdef __cplusplus
    extern "C" {
#endif
    /* ESP-IDF Core Framework Includes */
    #include "esp_err.h"                // Error handling definitions and macro utilities
    #include "esp_log.h"                // System logging macros (ESP_LOGx)
    #include "esp_system.h"             // Core system APIs and reset control

    /* FreeRTOS Kernel Includes */
    #include "freertos/FreeRTOS.h"      // Core FreeRTOS configuration and types
    #include "freertos/task.h"          // Task management and delay API
    #include "freertos/queue.h"         // Thread-safe queue API
    #include "freertos/semphr.h"        // Semaphore and mutex utilities
    #include "freertos/event_groups.h"  // Event flag synchronization mechanism
#ifdef __cplusplus
}
#endif
