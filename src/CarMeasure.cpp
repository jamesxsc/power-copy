//
// Created by James Conway on 20/11/2024.
//

#include <cmath>
#include <vector>
#include <numeric>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <cstring>
#include "CarMeasure.h"
#include "esp_adc/adc_continuous.h"

CarMeasure::CarMeasure() {
}

static TaskHandle_t s_task_handle = nullptr;

// temp test
static bool IRAM_ATTR
s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data) {
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    if (s_task_handle)
        vTaskNotifyGiveFromISR(s_task_handle, &mustYield);
    // todo note that this is very wrong - the relevant task is the measure task not init context/main

    return (mustYield == pdTRUE);
}

void CarMeasure::init() {
    adc_continuous_handle_cfg_t adc_config = {
            .max_store_buf_size = 2048,
            // below is num of byes ie num of samples * 2
            .conv_frame_size = 512 // todo consider macro for numsize so we can use in array init
    };
    esp_err_t err = adc_continuous_new_handle(&adc_config, &adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE("CarMeasure", "Error in adc_continuous_new_handle");
        // todo handle error
    }

    adc_continuous_config_t adc_dig_config = {
            .pattern_num = 1,
            .sample_freq_hz = 20000, // todo we probably need way more samples at this rate
            .conv_mode = ADC_CONV_SINGLE_UNIT_1,
            .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
    };

    adc_digi_pattern_config_t adc_pattern[1] = {
            {
                    .atten = ADC_ATTEN_DB_12,
                    .channel = ADC_CHANNEL_0,
                    .unit = ADC_UNIT_1,
                    .bit_width = ADC_BITWIDTH_12
            }
    };
    adc_dig_config.adc_pattern = adc_pattern;

    err = adc_continuous_config(adc_handle, &adc_dig_config);
    if (err != ESP_OK) {
        ESP_LOGE("CarMeasure", "Error in adc_continuous_config");
        // todo handle error
    }

    adc_continuous_evt_cbs_t adc_cbs = {
            .on_conv_done = s_conv_done_cb
    };
    err = adc_continuous_register_event_callbacks(adc_handle, &adc_cbs, nullptr);
    if (err != ESP_OK) {
        ESP_LOGE("CarMeasure", "Error in adc_continuous_register_event_callbacks");
        return;
        // todo handle error
    }

    err = adc_continuous_start(adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE("CarMeasure", "Error in adc_continuous_start");
        // todo handle error
    }

}

int CarMeasure::measureRMSAmperes() {

    ESP_LOGI("CarMeasure", "Measuring RMS Amperes");

    s_task_handle = xTaskGetCurrentTaskHandle();

    int numSamples = 256;
    uint8_t result[512] = {0}; // num of bytes
    uint32_t ret_num = 0;
    memset(result, 0, numSamples);

    std::vector<int> samples;
    samples.reserve(numSamples);

    esp_err_t err = adc_continuous_read(adc_handle, result, numSamples * 2, &ret_num, 0);
    if (err != ESP_OK) {
        ESP_LOGE("CarMeasure", "Error in adc_continuous_read");
        // print error type
        ESP_LOGE("CarMeasure", "Error type: %d", err);
        return 0;
        // todo handle error
    }

    // note there are two bytes per sample
    // this should be 'safe' ie worst case we get less samples, but may need to update for later calcs
    for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
        ESP_LOGI("CarMeasure", "Sample %d: %d", i, ((adc_digi_output_data_t *) &result[i])->type1.data);
        samples.push_back(((adc_digi_output_data_t *) &result[i])->type1.data);
    }

    // print vector size
    ESP_LOGI("CarMeasure", "Vector size: %d", samples.size());

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
