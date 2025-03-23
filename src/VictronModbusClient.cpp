//
// Created by James Conway on 20/11/2024.
//

#include "VictronModbusClient.h"
#include <esp_log.h>
#include <esp_netif_sntp.h>
#include "ErrorHandler.h"
#include "esp_sntp.h"

VictronModbusClient::VictronModbusClient(const std::string &host, int port) : host_(host), port_(port) {
    mb_ = nullptr;
    windowEndTaskHandle_ = nullptr;
}

void VictronModbusClient::init() {
    mb_ = new modbus(host_, port_);
    mb_->modbus_set_slave_id(100);
    mb_->modbus_connect();

    if (mb_->err || !mb_->is_connected()) {
        ESP_LOGE("VictronModbusClient", "Modbus connection failed");
        ErrorHandler::error();
        return;
    }

    ESP_LOGI("VictronModbusClient", "Modbus connection successful");

    ESP_LOGI("VictronModbusClient", "Initializing NTP");

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);

    // todo check this it's a little flaky
    esp_err_t err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000));
    if (err != ESP_OK) {
        ESP_LOGE("VictronModbusClient", "Error waiting for NTP to initialize");
        ErrorHandler::error();
        return;
    }

    setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0", 1);
    tzset();
}

time_t dontCheckTill = 0;
bool charging = false;

void VictronModbusClient::handleCarSignal() {
    time_t now;
    struct tm timeinfo{};
    time(&now);
    localtime_r(&now, &timeinfo);

    uint8_t currentHour = timeinfo.tm_hour;
    uint8_t currentMins = timeinfo.tm_min;
    ESP_LOGI("VictronModbusClient", "Current time: %d:%d", currentHour, currentMins);

    uint8_t windowEndHour;
    uint8_t windowEndMins;

    // todo ignore first 2 minutes of windows in case car runs over

    if (currentMins < 30) {
        windowEndHour = currentHour;
        windowEndMins = 30;
    } else {
        windowEndHour = currentHour + 1;
        windowEndMins = 0;
    }
    ESP_LOGI("VictronModbusClient", "Window end time: %d:%d", windowEndHour, windowEndMins);

    if (currentMins <= 2 || (currentMins <= 32 && currentMins >= 30)) {
        ESP_LOGI("VictronModbusClient", "Ignoring first 2 minutes of window");
        return;
    }

    int secondsTillNextHalfHour = 60 * (windowEndMins - currentMins) + 60 * (windowEndHour - currentHour);
    ESP_LOGI("VictronModbusClient", "Seconds till end of window: %d", secondsTillNextHalfHour);
    dontCheckTill = now + secondsTillNextHalfHour;

    modbusChargeToFull();
}

void VictronModbusClient::handleNoCarSignal() {
    // check that the current window has ended
    ESP_LOGI("VictronModbusClient", "Checking if the current window has ended before resuming normal operation");

    time_t now;
    time(&now);

    if (now > dontCheckTill) {
        ESP_LOGI("VictronModbusClient", "Current window has ended, resuming normal operation");
        modbusNormalOperation();
    }
}

void VictronModbusClient::modbusNormalOperation() {
    if (!charging) {
        return;
    }

    uint16_t registerValue = 0;

    while (registerValue != 10) {
        ESP_LOGI("VictronModbusClient", "Attempting to write register for modbusNormalOperation");
        mb_->modbus_write_register(2900, 10);

        if (mb_->err) {
            ESP_LOGE("VictronModbusClient", "Error writing to register in modbusNormalOperation");
            ESP_LOGE("VictronModbusClient", "Error message: %s", mb_->error_msg.c_str());
            ErrorHandler::error();
            return;
        }

        mb_->modbus_read_holding_registers(2900, 1, &registerValue);
    }

    // If we don't error update internal state
    charging = false;

    ESP_LOGI("VictronModbusClient", "Modbus normal operation command successful");
}

void VictronModbusClient::modbusChargeToFull() {
    if (charging) {
        return;
    }

    uint16_t registerValue = 0;

    while (registerValue != 9) {
        ESP_LOGI("VictronModbusClient", "Attempting to write register for modbusChargeToFull");
        mb_->modbus_write_register(2900, 9);

        if (mb_->err) {
            ESP_LOGE("VictronModbusClient", "Error writing to register in modbusChargeToFull");
            ESP_LOGE("VictronModbusClient", "Error message: %s", mb_->error_msg.c_str());
            ErrorHandler::error();
            return;
        }

        mb_->modbus_read_holding_registers(2900, 1, &registerValue);
    }

    // If we don't error update internal state
    charging = true;

    ESP_LOGI("VictronModbusClient", "Modbus charge to full command successful");
}

VictronModbusClient::~VictronModbusClient() {
    mb_->modbus_close();
    delete mb_;
}
