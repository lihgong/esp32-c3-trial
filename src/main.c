#include<stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/twai.h"

void app_main()
{
    extern void test_print(void);
    extern void test_twai(void);
    extern void test_sd_spi(void);

    //test_print();
    test_sd_spi();
    test_twai();
}

