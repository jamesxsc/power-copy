//
// Created by James Conway on 20/11/2024.
//

#include <cmath>
#include <vector>
#include <numeric>
#include <esp_log.h>
#include "CarMeasure.h"
#include "driver/adc.h"

CarMeasure::CarMeasure() {
}

void CarMeasure::init() {

    // todo alter to use this https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/adc_continuous.html

    // Config ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);
}

int CarMeasure::measureRMSAmperes() {
    int numSamples = 256;

    std::vector<int> samples;
    samples.reserve(numSamples);

    for (int i = 0; i < numSamples; i++) {
        samples.push_back(adc1_get_raw(ADC1_CHANNEL_0));
//        printf("%d,%d\n", i, samples[i]); // this delay is helping a lot, should be fixed when we continuous reaad
    }

    int dc = std::reduce(samples.begin(), samples.end(), 0) / numSamples;

    ESP_LOGI("CarMeasure", "DC: %d", dc);

    int rmsCountsSquared = std::transform_reduce(
            samples.begin(), samples.end(), 0, std::plus<>(),
            [dc](int value) { return (value - dc) * (value - dc); }
    ) / numSamples;


    ESP_LOGI("CarMeasure", "RMS Counts Squared: %d", rmsCountsSquared);

    // 1000mV * voltCal V for the range of 4096 ADC counts.
    // Then use I  = V/R and the coil ratio

    double milliVolts = ((double) voltCal / 4096 * sqrt(rmsCountsSquared));

    ESP_LOGI("CarMeasure", "MilliVolts: %f", milliVolts);

    double current = milliVolts / res;

    ESP_LOGI("CarMeasure", "Current: %f", current);

    return (int) (coilRatio * (double) milliVolts / res);
    // todo tidy up types and rounding, but try to avoid float context?
}
