//
// Created by James Conway on 20/11/2024.
//

#pragma once

#include <string>
#include <esp_event.h>

class WiFiStation {

public:
    WiFiStation(std::string ssid, std::string password);

private:
    static void eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

};
