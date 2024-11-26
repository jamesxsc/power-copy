//
// Created by James Conway on 20/11/2024.
//

#pragma once

#include "esp_adc/adc_continuous.h"

class CarMeasure {

public:
    void init();

    int measureRMSAmperes();

private:
    int res_ = 24;
    int voltCal_ = 2840; // Obtained from attenuation at 12dB giving 3.55 * 800mV
    int coilRatio_ = 100000 / 50; // 100A - 50mA

    adc_continuous_handle_t adc_handle_ = nullptr;

};
