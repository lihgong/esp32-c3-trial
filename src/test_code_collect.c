#include<stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/twai.h"

void test_print(void)
{
    uint32_t i=0;
    while(1) {
        printf("Hello World ABC %d\n", i++);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


void test_twai(void)
{
    esp_err_t ret;

    // Install TWAI driver
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_0, GPIO_NUM_1, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config  = TWAI_TIMING_CONFIG_125KBITS(); // TWAI_TIMING_CONFIG_125KBITS(); // TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    ret = twai_driver_install(&g_config, &t_config, &f_config);
    printf("twai_driver_install() return %d\n", ret);
    ESP_ERROR_CHECK(ret);

    // START ESP TWAI state machine, this must be called to start TWAI
    ret = twai_start();
    printf("twai_start(), return %d\n", ret);
    ESP_ERROR_CHECK(ret);

    while(1) { 
        twai_message_t msg;
        if(twai_receive(&msg, pdMS_TO_TICKS(1000)) != ESP_OK) { // Wait for message to be received
            continue;
        }

        // Process received message
        printf("> %03X [%d] ", msg.identifier, msg.data_length_code);
        for(int i=0; i<msg.data_length_code; i++) {
            printf("%02X ", msg.data[i]);
        }
        printf("\n");

        // Send back the CAN message with ID+1 back to bus
        msg.identifier += 1;
        if (twai_transmit(&msg, pdMS_TO_TICKS(30)) == ESP_OK) {
        } else {
        }
    }
}
