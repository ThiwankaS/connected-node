/**
 * @file app_main.hpp
 * @brief ESP-IDF entry-point bridge for C++ application classes.
 *
 * ESP-IDF expects a C symbol `app_main()`. Application logic is implemented in C++
 * classes; this header provides a small macro that generates the required C entry
 * point and forwards execution to a static application instance.
 */

#pragma once

/**
 * @def ESP32_APP_MAIN
 * @brief Expands to ESP-IDF `app_main()` that runs a C++ application object.
 *
 * The macro creates one static instance of @p App and calls its `run()` method.
 * `run()` should perform short startup work (GPIO init, task creation) and return
 * quickly so the FreeRTOS `main` task does not block the CPU or trigger the
 * Task Watchdog Timer (TWDT).
 *
 * @param App C++ type with a public `void run()` method (e.g. uart::Console).
 *
 * @code
 * class MyApp { public: void run(); };
 * ESP32_APP_MAIN(MyApp);
 * @endcode
 */
#define ESP32_APP_MAIN(App)             \
    extern "C" void app_main(void) {    \
        static App app;                 \
        app.run();                      \
    }
