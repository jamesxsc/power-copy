#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "CarMeasure.h"
#include "VictronModbusClient.h"

constexpr int threshold = 500; // 500mA

CarMeasure carMeasure;
VictronModbusClient modbusClient("10.0.0.116", 502);

extern "C" void app_main() {

    // turn on gpio2 led
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 1);

    xTaskCreate([](void* arg) {
        for (;;) {
            ESP_LOGI("main", "Taking measurement and posting");

            int measurement = carMeasure.measureRMSAmperes();

            ESP_LOGI("main", "Measurement: %d", measurement);

            if (measurement > threshold) {
                // todo signal to modbusclient to initiate 1/2 hour of on time
            }

            // TODO what is provider's mininum window size such that we capture all half hour intervals
            vTaskDelay(5000 / portTICK_PERIOD_MS); // Temp 5 seconds
        }
    }, "main_task", 2048, nullptr, 5, nullptr);

}
