#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "CarMeasure.h"
#include "VictronModbusClient.h"
#include "WiFiStation.h"
#include "ErrorHandler.h"

constexpr int threshold = 1500; // 500mA

const gpio_num_t ledPin = GPIO_NUM_2;

CarMeasure carMeasure;
VictronModbusClient modbusClient("10.0.3.25", 502);
WiFiStation wifiStation("Futura", "");

extern "C" void app_main() {
    ErrorHandler::init(ledPin);

    // Short delay for wifi
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    wifiStation.init(); // this (nvs) has to be done before ADC init
    carMeasure.init();
    modbusClient.init();

    // todo stop here if an error is encountered

    ErrorHandler::ok();

    xTaskCreate([](void* arg) {
        for (;;) {
            vTaskDelay(5000 / portTICK_PERIOD_MS); // Temp 5 seconds

            ESP_LOGI("main", "Taking measurement and posting");

            uint32_t measurement = carMeasure.measureRMSAmperes();

            ESP_LOGI("main", "Measurement: %lu", measurement);

            if (measurement > threshold) {
                modbusClient.handleCarSignal();
            }

            // TODO what is provider's mininum window size such that we capture all half hour intervals
        }
    }, "main_task", 8192, nullptr, 5, nullptr);

}
