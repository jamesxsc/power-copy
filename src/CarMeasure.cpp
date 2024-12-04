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
#include "ErrorHandler.h"

static TaskHandle_t s_task_handle = nullptr;

#define NUM_SAMPLES 2048

// first thing to try is removing this altogether
// if not we pass instance of main task from initializer
//static bool IRAM_ATTR
//s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data) {
//    BaseType_t mustYield = pdFALSE;
//    Notify that ADC continuous driver has done enough number of conversions
//    if (s_task_handle)
//        vTaskNotifyGiveFromISR(s_task_handle, &mustYield);
//     todo note that this is very wrong - the relevant task is the measure task not init context/main
//
//    return (mustYield == pdTRUE);
//}

void CarMeasure::init() {
    adc_continuous_handle_cfg_t adc_config = {
            .max_store_buf_size = 2048,
            // below is num of byes ie num of samples * 2
            .conv_frame_size = NUM_SAMPLES * 2
    };
    esp_err_t err = adc_continuous_new_handle(&adc_config, &adc_handle_);
    if (err != ESP_OK) {
        ESP_LOGE("CarMeasure", "Error in adc_continuous_new_handle");
        ErrorHandler::error();
        return;
    }

    adc_continuous_config_t adc_dig_config = {
            .pattern_num = 1,
            .sample_freq_hz = 20000, // 1 cycle at 50Hz = 400 samples = 20ms - use buffer 2048 (see below)
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

    err = adc_continuous_config(adc_handle_, &adc_dig_config);
    if (err != ESP_OK) {
        ESP_LOGE("CarMeasure", "Error in adc_continuous_config");
        ErrorHandler::error();
        return;
    }

//    adc_continuous_evt_cbs_t adc_cbs = {
//            .on_conv_done = s_conv_done_cb
//    };
//
//    err = adc_continuous_register_event_callbacks(adc_handle_, &adc_cbs, nullptr);
    if (err != ESP_OK) {
        ESP_LOGE("CarMeasure", "Error in adc_continuous_register_event_callbacks");
        ErrorHandler::error();
        return;
    }

    err = adc_continuous_start(adc_handle_);
    if (err != ESP_OK) {
        ESP_LOGE("CarMeasure", "Error in adc_continuous_start");
        ErrorHandler::error();
        return;
    }

}

// There is still some noise in the readings
// But it is good enough for our threshold check
// In the future, amplification at the input *could* improve this
uint32_t CarMeasure::measureRMSAmperes() {
    ESP_LOGV("CarMeasure", "Measuring RMS Amperes");

    s_task_handle = xTaskGetCurrentTaskHandle();

    size_t numSamples = NUM_SAMPLES;
    uint8_t result[NUM_SAMPLES * 2] = {0}; // num of bytes
    uint32_t ret_num = 0;
    memset(result, 0, numSamples * 2);

    std::vector<int> samples;
    samples.reserve(numSamples);

    esp_err_t err = adc_continuous_read(adc_handle_, result, numSamples * 2, &ret_num, 0);
    if (err != ESP_OK) {
        ESP_LOGE("CarMeasure", "Error in adc_continuous_read");
        ErrorHandler::error();
        return 0;
    }

    // note there are two bytes per sample
    // this should be 'safe' ie worst case we get less samples, but may need to update for later calcs
    for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
        samples.push_back(((adc_digi_output_data_t *) &result[i])->type1.data);
    }

    // ensure that numsamples is correct for calculations below
    numSamples = samples.size();

    // print vector size
    ESP_LOGI("CarMeasure", "Vector size: %d", samples.size());

    uint32_t dc = std::reduce(samples.begin(), samples.end(), 0) / numSamples;

    ESP_LOGI("CarMeasure", "DC: %lu", dc);

    uint32_t rmsCountsSquared = std::transform_reduce(
            samples.begin(), samples.end(), 0, std::plus<>(),
            [dc](int value) { return (value - dc) * (value - dc); }
    ) / numSamples;


    ESP_LOGI("CarMeasure", "RMS Counts Squared: %lu", rmsCountsSquared);

    // 1000mV * voltCal V for the range of 4096 ADC counts.
    // Then use I  = V/R and the coil ratio

    uint32_t milliVolts =  voltCal_ * (uint32_t) sqrt(rmsCountsSquared) / 4096;

    ESP_LOGI("CarMeasure", "MilliVolts: %lu", milliVolts);

    uint32_t current = milliVolts / res_;

    ESP_LOGI("CarMeasure", "Current: %lu", current);

    uint32_t mainsCurrent = coilRatio_ * milliVolts / res_;

    // Sanity check
    if (mainsCurrent == 0 || mainsCurrent > 100000) {
        ErrorHandler::error();
        return 0;
    }

    return mainsCurrent;
}
