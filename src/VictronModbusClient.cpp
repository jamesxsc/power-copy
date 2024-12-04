//
// Created by James Conway on 20/11/2024.
//

#include "VictronModbusClient.h"
#include <chrono>
#include <ctime>
#include <esp_log.h>
#include "freertos/task.h"
#include "ErrorHandler.h"

struct task_params_t {
    VictronModbusClient *instance;
    int millis;
};

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

}

// TODO check summer time
void VictronModbusClient::handleCarSignal() {
    modbusChargeToFull();

    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    time_t time = std::chrono::system_clock::to_time_t(tp);
    tm local_tm = *localtime(&time);

    uint8_t currentHour = local_tm.tm_hour;
    uint8_t currentMins = local_tm.tm_min;

    uint8_t windowEndHour;
    uint8_t windowEndMins;
    if (currentMins < 30) {
        windowEndHour = currentHour;
        windowEndMins = 30;
    } else {
        windowEndHour = currentHour + 1;
        windowEndMins = 0;
    }

    eTaskState state = eTaskGetState(*windowEndTaskHandle_);
    // This means that the task has finished, so we are in the next time interval and want to start a new task for the end
    if (state == eDeleted || state == eInvalid) {
    int millisTillTask = 60000 * ((windowEndHour - currentHour) * 60 + windowEndMins - currentMins);
    task_params_t params = {
            .instance = this,
            .millis = millisTillTask
    };
    xTaskCreate([](void *args) {
        auto *params = (task_params_t *) args;
        vTaskDelay(params->millis);
        params->instance->modbusNormalOperation();
    }, "window_end_task", 2048, (void *) &params, 5, windowEndTaskHandle_);
    }

}

void VictronModbusClient::modbusNormalOperation() {
    mb_->modbus_write_register(2900, 10);

    if (mb_->err) {
        ESP_LOGE("VictronModbusClient", "Error writing to register in modbusNormalOperation");
        ErrorHandler::error();
        return;
    }
}

void VictronModbusClient::modbusChargeToFull() {
    mb_->modbus_write_register(2900, 9);

    if (mb_->err) {
        ESP_LOGE("VictronModbusClient", "Error writing to register in modbusChargeToFull");
        ErrorHandler::error();
        return;
    }
}

VictronModbusClient::~VictronModbusClient() {
    mb_->modbus_close();
    delete mb_;
    delete windowEndTaskHandle_;
}
