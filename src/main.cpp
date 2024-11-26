#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "CarMeasure.h"
#include "VictronModbusClient.h"
#include "WiFiStation.h"
#include "ErrorHandler.h"

constexpr int threshold = 500; // 500mA

const gpio_num_t ledPin = GPIO_NUM_2;

CarMeasure carMeasure;
VictronModbusClient modbusClient("10.0.0.116", 502);
WiFiStation wifiStation("Futura", "");

extern "C" void app_main() {

    ErrorHandler::init(ledPin);

    wifiStation.init(); // this (nvs) has to be done before ADC init
    carMeasure.init();
    modbusClient.init();

    ErrorHandler::ok();

    xTaskCreate([](void* arg) {
        for (;;) {
            vTaskDelay(5000 / portTICK_PERIOD_MS); // Temp 5 seconds

            ESP_LOGI("main", "Taking measurement and posting");

            int measurement = carMeasure.measureRMSAmperes();

            ESP_LOGI("main", "Measurement: %d", measurement);

            if (measurement > threshold) {
                modbusClient.handleCarSignal();
            }

            // TODO what is provider's mininum window size such that we capture all half hour intervals
        }
    }, "main_task", 8192, nullptr, 5, nullptr);

}
