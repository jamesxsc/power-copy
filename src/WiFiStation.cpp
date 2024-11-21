//
// Created by James Conway on 20/11/2024.
//

#include "WiFiStation.h"
#include "esp_wifi.h"

// Once instantiated, this class is responsible for maintaining the (critical) to allow Modbus communication with Cerbo GX
WiFiStation::WiFiStation(std::string ssid, std::string password) {
    esp_err_t err = ESP_OK;

    // WiFi init

    EventGroupHandle_t wifi_event_group = xEventGroupCreate();

    err = esp_netif_init();

    if (err != ESP_OK) {
        // TODO handle error
    }

    err = esp_event_loop_create_default();

    if (err != ESP_OK) {
        // TODO handle error
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

    err = esp_wifi_init(&config);

    if (err != ESP_OK) {
        // TODO handle error
    }

    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFiStation::eventHandler, nullptr, nullptr);

    if (err != ESP_OK) {
        // TODO handle error
    }

    err = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &WiFiStation::eventHandler, nullptr, nullptr);

    if (err != ESP_OK) {
        // TODO handle error
    }

    wifi_config_t wifi_config = {
            .sta = {
                    .ssid = {*(reinterpret_cast<const uint8_t *>(&ssid[0]))},
                    .password = {*(reinterpret_cast<const uint8_t *>(&password[0]))}
            }
    };

    err = esp_wifi_set_mode(WIFI_MODE_STA);

    if (err != ESP_OK) {
        // TODO handle error
    }

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    if (err != ESP_OK) {
        // TODO handle error
    }

    err = esp_wifi_start();

    if (err != ESP_OK) {
        // TODO handle error
    }

    // todo read event bits to determine success or fail





}

void WiFiStation::eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // todo consider retry logic and fail flag
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // todo set event bit
        // basically this signifies connection success
    }
}
