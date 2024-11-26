//
// Created by James Conway on 20/11/2024.
//

#pragma once

#include <string>
#include "../lib/modbus.h"

class VictronModbusClient {

public:

    VictronModbusClient(const std::string& host, int port);

    void init();

    void handleCarSignal();

    ~VictronModbusClient();

private:
    std::string host_;
    int port_;
    modbus* mb_;
    TaskHandle_t* windowEndTaskHandle_;

    void modbusChargeToFull();
    void modbusNormalOperation();

};
