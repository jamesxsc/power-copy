//
// Created by James Conway on 20/11/2024.
//

#include <cmath>
#include <vector>
#include <numeric>
#include "CarMeasure.h"
#include "driver/adc.h"

CarMeasure::CarMeasure() {
    // todo alter to use this https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/adc_continuous.html

    // Config ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);
}

int CarMeasure::measureRMSAmperes() {
    int numSamples = 128;

    std::vector<int> samples;
    samples.reserve(numSamples);

    for (int i = 0; i < numSamples; i++) {
        samples.push_back(adc1_get_raw(ADC1_CHANNEL_0));
    }

    int mean = std::reduce(samples.begin(), samples.end(), 0) / numSamples;

    int rmsCountsSquared = std::transform_reduce(
            samples.begin(), samples.end(), 0, std::plus<>(),
            [mean](int value) { return (value - mean) * (value - mean); }
    ) / numSamples;

    // 1000mV * voltCal V for the range of 4096 ADC counts.
    // Then use I  = V/R and the coil ratio
    return  (int) ((double) coilRatio * voltCal / 4096 * sqrt(rmsCountsSquared) / res);
}
