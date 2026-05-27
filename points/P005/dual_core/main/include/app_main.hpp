#pragma once

#define ESP32_APP_MAIN(App)             \
    extern "C" void app_main(void) {    \
        static App app;                 \
        app.run();                      \
    }

