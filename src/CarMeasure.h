//
// Created by James Conway on 20/11/2024.
//

#pragma once

class CarMeasure {

public:

    CarMeasure();
    void init();

    int measureRMSAmperes();

private:
    int res = 24;
    int voltCal = 2840; // Obtained from attenuation at 12dB giving 3.55 * 800mV
    int coilRatio = 100000 / 50; // 100A - 50mA

};
