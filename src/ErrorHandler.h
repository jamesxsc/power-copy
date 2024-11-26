//
// Created by James Conway on 26/11/2024.
//

#pragma once

#include "driver/gpio.h"

class ErrorHandler {

public:
    static void init(gpio_num_t ledPin);

    static void ok();

    static void error();

};

