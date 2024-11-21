//
// Created by James Conway on 20/11/2024.
//

#include "VictronModbusClient.h"
#include <chrono>
#include <ctime>

struct task_params_t {
    VictronModbusClient *instance;
    int millis;
};

VictronModbusClient::VictronModbusClient(const std::string &host, int port) : host(host), port(port) {
    mb = new modbus(host, port);
    mb->modbus_set_slave_id(100);
    mb->modbus_connect();

    if (mb->err || mb->is_connected()) {
        // TODO handle error. we need a way to inform customer
    }
}

// TODO check summer time
// TODO check VRM logging capability and add our own if insufficient
// cost saving to customer needs to be obtainable
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

    TaskStatus_t status;
    vTaskGetInfo(*windowEndTaskHandle, &status, pdFALSE, eInvalid);
    // This means that the task has finished, so we are in the next time interval and want to start a new task for the end
    if (status.eCurrentState == eDeleted || status.eCurrentState == eInvalid) {
        int millisTillTask = 60000 * ((windowEndHour - currentHour) * 60 + windowEndMins - currentMins);
        task_params_t params = {
                .instance = this,
                .millis = millisTillTask
        };
        xTaskCreate([](void *args) {
            auto *params = (task_params_t *) args;
            vTaskDelay(params->millis);
            params->instance->modbusNormalOperation();
        }, "window_end_task", 2048, (void *) &params, 5, windowEndTaskHandle);
    }

}

VictronModbusClient::~VictronModbusClient() {
    mb->modbus_close();
    delete mb;
    delete windowEndTaskHandle;
}

void VictronModbusClient::modbusNormalOperation() {
    mb->modbus_write_register(2900, 10);
}

void VictronModbusClient::modbusChargeToFull() {
    mb->modbus_write_register(2900, 9);
}
