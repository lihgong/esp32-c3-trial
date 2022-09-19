#include<stdio.h>
#include<string.h>
#include<sys/unistd.h>
#include<sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

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


static const char *TAG = "example";

#define MOUNT_POINT "/sdcard"

// Pin mapping
#define PIN_NUM_MISO    (2)
#define PIN_NUM_MOSI    (7)
#define PIN_NUM_CLK     (6)
#define PIN_NUM_CS      (10)

#define SPI_DMA_CHAN    (SPI_DMA_CH_AUTO)

void test_sd_spi(void)
{
    esp_err_t ret;

    // SPI host configuration
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    // Initialize file system
    sdmmc_card_t *card;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5, // max open file
        .allocation_unit_size = 16 * 1024, // if mount fail and format en, use the size to format
    };
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s) Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);




    // POSIX and C standard API to work with file
    const char *file_hello = MOUNT_POINT"/hello.txt";

    ESP_LOGI(TAG, "Opening file %s", file_hello);
    FILE *f = fopen(file_hello, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Hello %s!\n", card->cid.name);

    // SPEED TEST of the SD card, write 128*32*1024 = 4MB byte to SD card
    for(uint32_t i=0; i<32*1024; i++) {
        fprintf(f, "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456\n");
    }
    fclose(f);
    ESP_LOGI(TAG, "File written");

    const char *file_foo = MOUNT_POINT"/foo.txt";

    // Check if destination file exists before renaming
    struct stat st;
    if (stat(file_foo, &st) == 0) {
        // Delete it if it exists
        unlink(file_foo);
    }

    // Rename original file
    ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    if (rename(file_hello, file_foo) != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }

    // Open renamed file for reading
    ESP_LOGI(TAG, "Reading file %s", file_foo);
    f = fopen(file_foo, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    // Read a line from file
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);

    // Strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    ESP_LOGI(TAG, "Card unmounted");

    //deinitialize the bus after all devices are removed
    spi_bus_free(host.slot);
}
