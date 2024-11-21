//
// Created by James Conway on 20/11/2024.
//

#pragma once

#include <string>
#include "../lib/modbus.h"

class VictronModbusClient {

public:

    VictronModbusClient(const std::string& host, int port);

    void handleCarSignal();

    ~VictronModbusClient();

private:
    std::string host;
    int port;
    modbus* mb;
    TaskHandle_t* windowEndTaskHandle;

    void modbusChargeToFull();
    void modbusNormalOperation();

};
