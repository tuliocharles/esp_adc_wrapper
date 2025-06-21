#include <stdio.h>
#include "esp_adc_wrapper.h"

void app_main(void)
{

    esp_adc_wrapper_config_t config = {
        .adc_channel = 0,          // ADC channel to read from
        .adc_handle = NULL, // ADC handle for ADC1;
    };

    esp_adc_wrapper_handle_t esp_adc_wrapper_handle;
    
    esp_err_t ret = init_esp_adc_wrapper(&config, &esp_adc_wrapper_handle);
    if (ret != ESP_OK) {
        printf("Failed to initialize ADC: %s\n", esp_err_to_name(ret));
        return;
    }
    printf("ADC initialized successfully\n");

    esp_adc_wrapper_config_t config2 = {
        .adc_channel = 1,          // ADC channel to read from
        .adc_handle = get_adc_handle(esp_adc_wrapper_handle), // ADC handle for ADC1;
    };

    esp_adc_wrapper_handle_t esp_adc_wrapper_handle2;
    
    esp_err_t ret2 = init_esp_adc_wrapper(&config2, &esp_adc_wrapper_handle2);
    
    if (ret2 != ESP_OK) {
        printf("Failed to initialize ADC: %s\n", esp_err_to_name(ret));
        return;
    }
    printf("ADC initialized successfully\n");

    int raw_value = 0;
    int voltage_value = 0;
    while (1) {
        read_esp_adc_wrapper(esp_adc_wrapper_handle, &raw_value, &voltage_value);
        printf("ADC Raw Value: %d, Voltage Value: %d mV\n", raw_value, voltage_value);
        read_esp_adc_wrapper(esp_adc_wrapper_handle2, &raw_value, &voltage_value);
        printf("ADC Raw Value 2: %d, Voltage Value 2: %d mV\n", raw_value, voltage_value);
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }

    
    
}
