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

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5, // max open file
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ESP_LOGI(TAG, "Using SPI peripheral");
#if 0
typedef struct {
    uint32_t flags;             /*!< flags defining host properties */
#define SDMMC_HOST_FLAG_1BIT    BIT(0)      /*!< host supports 1-line SD and MMC protocol */
#define SDMMC_HOST_FLAG_4BIT    BIT(1)      /*!< host supports 4-line SD and MMC protocol */
#define SDMMC_HOST_FLAG_8BIT    BIT(2)      /*!< host supports 8-line MMC protocol */
#define SDMMC_HOST_FLAG_SPI     BIT(3)      /*!< host supports SPI protocol */
#define SDMMC_HOST_FLAG_DDR     BIT(4)      /*!< host supports DDR mode for SD/MMC */
#define SDMMC_HOST_FLAG_DEINIT_ARG BIT(5)      /*!< host `deinit` function called with the slot argument */
    int slot;                   /*!< slot number, to be passed to host functions */
    int max_freq_khz;           /*!< max frequency supported by the host */
#define SDMMC_FREQ_DEFAULT      20000       /*!< SD/MMC Default speed (limited by clock divider) */
#define SDMMC_FREQ_HIGHSPEED    40000       /*!< SD High speed (limited by clock divider) */
#define SDMMC_FREQ_PROBING      400         /*!< SD/MMC probing speed */
#define SDMMC_FREQ_52M          52000       /*!< MMC 52MHz speed */
#define SDMMC_FREQ_26M          26000       /*!< MMC 26MHz speed */
    float io_voltage;           /*!< I/O voltage used by the controller (voltage switching is not supported) */
    esp_err_t (*init)(void);    /*!< Host function to initialize the driver */
    esp_err_t (*set_bus_width)(int slot, size_t width);    /*!< host function to set bus width */
    size_t (*get_bus_width)(int slot); /*!< host function to get bus width */
    esp_err_t (*set_bus_ddr_mode)(int slot, bool ddr_enable); /*!< host function to set DDR mode */
    esp_err_t (*set_card_clk)(int slot, uint32_t freq_khz); /*!< host function to set card clock frequency */
    esp_err_t (*do_transaction)(int slot, sdmmc_command_t* cmdinfo);    /*!< host function to do a transaction */
    union {
        esp_err_t (*deinit)(void);  /*!< host function to deinitialize the driver */
        esp_err_t (*deinit_p)(int slot);  /*!< host function to deinitialize the driver, called with the `slot` */
    };
    esp_err_t (*io_int_enable)(int slot); /*!< Host function to enable SDIO interrupt line */
    esp_err_t (*io_int_wait)(int slot, TickType_t timeout_ticks); /*!< Host function to wait for SDIO interrupt line to be active */
    int command_timeout_ms;     /*!< timeout, in milliseconds, of a single command. Set to 0 to use the default value. */
} sdmmc_host_t;

#define SDSPI_HOST_DEFAULT() {\
    .flags = SDMMC_HOST_FLAG_SPI | SDMMC_HOST_FLAG_DEINIT_ARG, \
    .slot = SDSPI_DEFAULT_HOST, \
    .max_freq_khz = SDMMC_FREQ_DEFAULT, \
    .io_voltage = 3.3f, \
    .init = &sdspi_host_init, \
    .set_bus_width = NULL, \
    .get_bus_width = NULL, \
    .set_bus_ddr_mode = NULL, \
    .set_card_clk = &sdspi_host_set_card_clk, \
    .do_transaction = &sdspi_host_do_transaction, \
    .deinit_p = &sdspi_host_remove_device, \
    .io_int_enable = &sdspi_host_io_int_enable, \
    .io_int_wait = &sdspi_host_io_int_wait, \
    .command_timeout_ms = 0, \
}

#endif
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    //host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
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

#if 0
#define SDSPI_DEVICE_CONFIG_DEFAULT() {\
    .host_id   = SDSPI_DEFAULT_HOST, \
    .gpio_cs   = GPIO_NUM_13, \
    .gpio_cd   = SDSPI_SLOT_NO_CD, \
    .gpio_wp   = SDSPI_SLOT_NO_WP, \
    .gpio_int  = GPIO_NUM_NC, \
}
#endif

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files.

    // First create a file.
    const char *file_hello = MOUNT_POINT"/hello.txt";

    ESP_LOGI(TAG, "Opening file %s", file_hello);
    FILE *f = fopen(file_hello, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    for(uint32_t i=0; i<128*1024; i++) {
        fprintf(f, "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456\n");
    }
    fprintf(f, "Hello %s!\n", card->cid.name);
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
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted");

    //deinitialize the bus after all devices are removed
    spi_bus_free(host.slot);
}