#pragma once

/**
 * @file app_main.hpp
 * @brief Macro utility to bridge the ESP-IDF C entry point to a C++ class implementation.
 */

/**
 * @brief Generates the standard C entry point function and binds it to a C++ class.
 * 
 * @param AppType The name of the C++ class that handles the application lifecycle.
 * 
 * Usage example:
 *   ESP32_APP_MAIN(HeartBeatApplication)
 */
#define EPS32_APP_MAIN(App)                                                                 \
    /* Force the compiler to use C linkage so ESP-IDF can locate the entry point */         \
    extern "C" void app_main(void) {                                                        \
        /* Allocate the application instance statically to prevent heap fragmentation */    \
        /* and ensure the instance survives for the entire duration of the runtime.   */    \
        static App app;                                                                     \
        /* Invoke the main entry method of the C++ object to begin execution */             \
        app.run();                                                                          \
    }                                   
