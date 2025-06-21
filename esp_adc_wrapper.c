#include <stdio.h>
#include "esp_adc_wrapper.h"





static const char *TAG = "ADC";

typedef struct esp_adc_wrapper_t esp_adc_wrapper_t;

struct esp_adc_wrapper_t{
    int adc_raw;
    int adc_voltage;
    adc_channel_t adc_channel;
    adc_atten_t adc_atten; // ADC attenuation
    adc_oneshot_unit_handle_t adc_handle;
    adc_cali_handle_t adc1_cali_handle;
    bool do_calibration;
    
};



/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

/*#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif
*/

//#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
//#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
/*    
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));
*/
//#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
//#endif
}



esp_err_t init_esp_adc_wrapper(esp_adc_wrapper_config_t *config, esp_adc_wrapper_handle_t *handle)
{
    esp_err_t ret = ESP_OK;
    esp_adc_wrapper_t *esp_adc_wrapper = NULL;
    ESP_GOTO_ON_FALSE(config && handle, ESP_ERR_INVALID_ARG, err, TAG, "Invalid arguments");
    esp_adc_wrapper = calloc(1, sizeof(esp_adc_wrapper_t));

    if (!esp_adc_wrapper) {
        ESP_LOGE(TAG, "Failed to allocate memory for esp_adc_wrapper");
        ret = ESP_ERR_NO_MEM;
        goto err;
    }

    esp_adc_wrapper->adc_channel = config->adc_channel;
    esp_adc_wrapper->adc_handle = config->adc_handle;
    esp_adc_wrapper->adc_atten = ADC_ATTEN_DB_12; // Default attenuation

    if (esp_adc_wrapper->adc_handle == NULL) {
        ESP_LOGI(TAG, "ADC INITING FOR THE FIRST TIME");
        adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &esp_adc_wrapper->adc_handle));
    }
    
    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config_oneshot = {
        .atten = esp_adc_wrapper->adc_atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(esp_adc_wrapper->adc_handle, esp_adc_wrapper->adc_channel, &config_oneshot));

    //-------------ADC1 Calibration Init---------------//
    esp_adc_wrapper->do_calibration  = example_adc_calibration_init(ADC_UNIT_1, esp_adc_wrapper->adc_channel, esp_adc_wrapper->adc_atten, &esp_adc_wrapper->adc1_cali_handle);

    *handle = esp_adc_wrapper;
    ESP_LOGI(TAG, "ADC initialized successfully");
    ret =  ESP_OK;
    return ret;
err:
    if (esp_adc_wrapper) {
        free(esp_adc_wrapper);
        esp_adc_wrapper = NULL;
    }

    ESP_LOGE(TAG, "Failed to initialize adc: %s", esp_err_to_name(ret));
    return ret;

}


void read_esp_adc_wrapper(esp_adc_wrapper_handle_t handle, int *raw_value, int *voltage_value) {
    
    ESP_ERROR_CHECK(adc_oneshot_read(handle->adc_handle, handle->adc_channel, &handle->adc_raw));
    ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, handle->adc_channel, handle->adc_raw);

    if (handle->do_calibration) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(handle->adc1_cali_handle, handle->adc_raw, &handle->adc_voltage));
            ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, handle->adc_channel, handle->adc_voltage);
    }

    *raw_value = handle->adc_raw;
    *voltage_value = handle->adc_voltage;
    
}

void deinit_esp_adc_wrapper(esp_adc_wrapper_handle_t handle) {
    if (handle == NULL) {
        ESP_LOGE(TAG, "ADC handle is NULL");
        return;
    }

    if (handle->do_calibration) {
        example_adc_calibration_deinit(handle->adc1_cali_handle);
    }

    ESP_ERROR_CHECK(adc_oneshot_del_unit(handle->adc_handle));
    free(handle);
    handle = NULL;
    ESP_LOGI(TAG, "ADC deinitialized successfully");
}

// retorna handle do adc
adc_oneshot_unit_handle_t get_adc_handle(esp_adc_wrapper_handle_t handle) {
    if (handle == NULL) {
        ESP_LOGE(TAG, "ADC handle is NULL");
        return NULL;
    }
    return handle->adc_handle;
}
