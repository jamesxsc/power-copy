//
// Created by James Conway on 20/11/2024.
//

#pragma once

class CarMeasure {

public:

    CarMeasure();

    int measureRMSAmperes();

private:
    int res = 23;
    int voltCal = 3300;
    int coilRatio = 100000 / 50; // 100A - 50mA

};
