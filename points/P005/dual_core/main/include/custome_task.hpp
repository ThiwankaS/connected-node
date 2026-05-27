#pragma once

#include <cstdint>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/queue.h"
    #include "freertos/task.h"
}

namespace custome {
    class Task {
        public:
            template<typename T, void (T::*Loop)()>
            static void start (const char* name, uint32_t stack_word, UBaseType_t priority, T *self) {
                xTaskCreate(&trampoline<T, Loop>, name, stack_word, self, priority, nullptr);
            }
        private:
            template<typename T, void (T::*Loop)()>
            static void trampoline (void* arg) {
                (static_cast<T *>(arg)->*Loop)();
            }
    };
}
