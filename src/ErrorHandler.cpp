//
// Created by James Conway on 26/11/2024.
//

#include "ErrorHandler.h"
#include "freertos/FreeRTOS.h"

static gpio_num_t ledPin_;

void ErrorHandler::init(gpio_num_t ledPin) {
    ledPin_ = ledPin;
    gpio_set_direction(ledPin_, GPIO_MODE_OUTPUT);
}

void ErrorHandler::ok() {
    // turn on gpio2 led
    gpio_set_level(ledPin_, 1);
}

void ErrorHandler::error() {
    // cancel main task
    TaskHandle_t mainHandle = xTaskGetHandle("main_task");
    vTaskDelete(mainHandle);

    // flashing led error state persists for further investigation
    // in future we could post a description to a server
    // TODO this is on roadmap, along with logging cost saving for customer
    xTaskCreate([](void* arg) {
        for (;;) {
            gpio_set_level(ledPin_, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_set_level(ledPin_, 0);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }, "error_task", 2048, nullptr, 5, nullptr);
}
