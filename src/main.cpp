#include "esp_log.h"
#include "driver/gpio.h"

extern "C" void app_main() {

    // turn on gpio2 led
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 1);

    ESP_LOGI("main", "Hello, world!");

}
