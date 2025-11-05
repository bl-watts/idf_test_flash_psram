/**
    Test potential confligts between PSRAM and flash

    The reason for this it the problem mentioned in the IDF docs

    https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/external-ram.html#restrictions

    This test will make two tasks, one the updates a flash disk and one that
   make extensive PSRAM test.

    We like to see how the MPU will handle this and if an error ocure, how the
   detects this
 */

#include "esp_err.h"
#include <esp_check.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_littlefs.h>
#include <esp_random.h>
#include <esp_system.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static const char* TAG = __FILE_NAME__;

// Write random byte to the entiere buffer, to ensure heavy trafic to the PSRAM over SPI
void psram_stresstest() {
    static uint8_t EXT_RAM_NOINIT_ATTR buffer[1024 * 1024];

    uint8_t byte = random() % 255;

    for (int n = 0; n < sizeof(buffer); n++)
        buffer[n] = byte;

    printf("filled buffer of size %d kB with 0x%02X\n", sizeof(buffer) / 1024, byte);
}

#define BUFFER_SIZE 1024
// write data to fs, fill in random data, check integrity
bool littlefs_stresstest(const char* fname, int total_count) {
    FILE* file = fopen(fname, "rb+");

    if (file == NULL) {
        ESP_LOGE(TAG, "error opening file '%s' : %s\n", fname, strerror(errno));
        return false;
    }

    int count = 0;
    for (; count < total_count; count++) {
        char buffer[BUFFER_SIZE];

        esp_fill_random(buffer, sizeof(buffer));

        long pos = ftell(file);

        if (fwrite(buffer, sizeof(buffer), 1, file) > 0) {
            char buffer_test[BUFFER_SIZE];

            if (0 == fseek(file, pos, SEEK_SET)) {
                if (fread(buffer_test, sizeof(buffer_test), 1, file) > 0) {
                    if (0 != memcmp(buffer, buffer_test, sizeof(buffer_test))) {
                        ESP_LOGE(TAG, "data not written to disk, properly");
                        break;
                    }

                    fflush(file); // This is needed in IDF or the possition gets messed up !
                } else
                    ESP_LOGW(TAG, "error reading data from '%s' : %s", fname, strerror(errno));
            }
        } else
            ESP_LOGW(TAG, "error writing data to '%s' : %s\n", fname, strerror(errno));
    }

    fclose(file);

    printf("wrote %u bytes of data to flash, successfully\n", BUFFER_SIZE * count);
    return true;
}

void littlefs_init() {
    esp_vfs_littlefs_conf_t conf = {.base_path = "/data",
                                    .partition_label = "storage",
                                    .format_if_mount_failed = true,
                                    .grow_on_mount = true,
                                    .dont_mount = false};

    ESP_ERROR_CHECK(esp_vfs_littlefs_register(&conf));
    size_t total = 0, used = 0;

    ESP_ERROR_CHECK(esp_littlefs_info(conf.partition_label, &total, &used));

    ESP_LOGI(TAG, "littlefs disk size total %ld kB (used %ld bytes)", total / 1024, used);
}

void app_main(void) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    printf("silicon revision %d, ", chip_info.revision);

    uint32_t size_flash_chip;
    esp_flash_get_size(NULL, &size_flash_chip);

    printf("%" PRIu32 "MB %s flash\n", size_flash_chip / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Free heap: %" PRIu32 " bytes\n", esp_get_free_heap_size());

    printf("Now we are starting the LittleFs Demo ...\n");

    littlefs_init();
    littlefs_stresstest("/data/dumpdata.txt", 256);
    psram_stresstest();
}
