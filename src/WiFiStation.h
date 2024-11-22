//
// Created by James Conway on 20/11/2024.
//

#pragma once

#include <string>
#include "esp_event.h"

class WiFiStation {

public:
    WiFiStation(const std::string &ssid, const std::string &password);

    void init();

private:
    static void eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

    std::string ssid;
    std::string password;

};
