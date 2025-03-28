//
// Created by James Conway on 26/11/2024.
//

#include <esp_log.h>
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
    ESP_LOGI("ErrorHandler", "Error state entered");

    // cancel main task
    TaskHandle_t mainHandle = xTaskGetHandle("main_task");
    if (mainHandle != nullptr) {
        vTaskDelete(mainHandle);
    }

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

    vTaskDelay(1500 / portTICK_PERIOD_MS); // give it 1500 millis to show then reboot - should allow recovery from most issues
    esp_restart();
}
