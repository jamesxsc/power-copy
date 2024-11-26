//
// Created by James Conway on 20/11/2024.
//

#include "WiFiStation.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "ErrorHandler.h"

// Once instantiated, this class is responsible for maintaining the (critical) to allow Modbus communication with Cerbo GX
WiFiStation::WiFiStation(const std::string &ssid, const std::string &password) : ssid_(ssid), password_(password) {
}

static EventGroupHandle_t wifi_event_group;

void WiFiStation::eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // todo consider retry logic to prevent infinite loop, but reset on connected
        xEventGroupSetBits(wifi_event_group, BIT1);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, BIT0);
    }
}

void WiFiStation::init() {
    esp_err_t err = ESP_OK;

    // NVS flash (required for WiFi) init
    err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            ESP_LOGE("WiFiStation", "Error in nvs_flash_erase");
            ErrorHandler::error();
            return;
        }
        err = nvs_flash_init();
    }

    if (err != ESP_OK) {
        ESP_LOGE("WiFiStation", "Error in nvs_flash_init");
        ErrorHandler::error();
        return;
    }

    // WiFi init
    wifi_event_group = xEventGroupCreate();

    err = esp_netif_init();

    if (err != ESP_OK) {
        ESP_LOGE("WiFiStation", "Error in esp_netif_init");
        ErrorHandler::error();
        return;
    }

    err = esp_event_loop_create_default();

    if (err != ESP_OK) {
        ESP_LOGE("WiFiStation", "Error in esp_event_loop_create_default");
        ErrorHandler::error();
        return;
    }

    esp_netif_set_hostname(esp_netif_create_default_wifi_sta(), "power-copy");

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

    err = esp_wifi_init(&config);

    if (err != ESP_OK) {
        ESP_LOGE("WiFiStation", "Error in esp_wifi_init");
        ErrorHandler::error();
        return;
    }

    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFiStation::eventHandler, nullptr,
                                              nullptr);

    if (err != ESP_OK) {
        ESP_LOGE("WiFiStation", "Error in esp_event_handler_instance_register");
        ErrorHandler::error();
        return;
    }

    err = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &WiFiStation::eventHandler, nullptr, nullptr);

    if (err != ESP_OK) {
        ESP_LOGE("WiFiStation", "Error in esp_event_handler_instance_register");
        ErrorHandler::error();
        return;
    }

    wifi_config_t wifi_config = {
            .sta = {
                    .ssid = {},
                    .password = {},
            }
    };

    std::copy(ssid_.begin(), ssid_.end(), wifi_config.sta.ssid);
    std::copy(password_.begin(), password_.end(), wifi_config.sta.password);

    err = esp_wifi_set_mode(WIFI_MODE_STA);

    if (err != ESP_OK) {
        ESP_LOGE("WiFiStation", "Error in esp_wifi_set_mode");
        ErrorHandler::error();
        return;
    }

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    if (err != ESP_OK) {
        ESP_LOGE("WiFiStation", "Error in esp_wifi_set_config");
        ErrorHandler::error();
        return;
    }

    err = esp_wifi_start();

    if (err != ESP_OK) {
        ESP_LOGE("WiFiStation", "Error in esp_wifi_start");
        ErrorHandler::error();
        return;
    }

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, BIT0 | BIT1, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & BIT0) {
        ESP_LOGI("WiFiStation", "WiFi connected to AP");
    } else if (bits & BIT1) {
        ESP_LOGE("WiFiStation", "WiFi failed to connect to AP");
        ErrorHandler::error();
        return;
    } else {
        ESP_LOGE("WiFiStation", "WiFi encountered an unexpected error");
        ErrorHandler::error();
    }


}
