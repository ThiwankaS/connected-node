/**
 * @file custome_task.hpp
 * @brief Template-based FreeRTOS task wrapper for C++ class member functions.
 * Provides a type-safe interface to spawn FreeRTOS tasks directly from non-static 
 * object member loops, eliminating standard boilerplate.
 */

#pragma once 

#include <cstdint>

extern "C" {
    #include "FreeRTOSConfig.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/queue.h"
    #include "freertos/task.h"
    #include "portmacro.h"
}

/**
 * @namespace custome
 * @brief Custom application and task management namespace.
 */
namespace custome {
    /**
     * @class Task
     * @brief Utility class providing static abstraction for wrapping C++ methods into FreeRTOS tasks.
     */
    class Task {
        public:
            /**
             * @function template
             * @brief Instantiates and registers a new C++ member function as a FreeRTOS task.
             * @tparam T The class type containing the desired execution loop.
             * @tparam Loop A non-static pointer to a member function of class T taking no arguments and returning void.
             * @param[in] name Descriptive name for the task (useful for debugging).
             * @param[in] stack_word The depth of the task stack specified in words (not bytes).
             * @param[in] priority The priority level at which the created task should execute.
             * @param[in] self A raw pointer to the instance (`this`) of class T on which the member loop should be executed.
             */
            template <typename T, void(T::*Loop)()>
            static void start(const char* name, uint32_t stack_word, UBaseType_t priority, T* self){
                xTaskCreate(&trampoline<T, Loop>, name, stack_word, self, priority, nullptr);
            }
        private:
            /**
             * @function template
             * @brief C-compatible bridge function used as an entry point for FreeRTOS.
             * FreeRTOS expects a `void (*)(void*)` function signature. This template unpacks 
             * the class instance pointer from the incoming raw argument and natively executes 
             * the designated member function loop.
             * @tparam T The target class type.
             * @tparam Loop The pointer to the instance member function.
             * @param[in] arg A generic pointer referencing the instance target (`self`).
             */
            template <typename T, void(T::*Loop)()>
            static void trampoline(void* arg) {
                (static_cast<T*>(arg)->*Loop)();
            }
    };
}
