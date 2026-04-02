#include "sd_storage.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_master.h"
#include "driver/sdspi_host.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "capture_config.h"

static const char *TAG = "sd_storage";

static bool s_mounted = false;
static sdmmc_card_t *s_card = NULL;
#if !PULSEMON_SD_USE_SDMMC
static bool s_bus_initialized = false;
#endif

#if PULSEMON_SD_USE_SDMMC
static bool sd_pins_valid(void)
{
    return (PULSEMON_SDMMC_PIN_CLK >= 0) &&
           (PULSEMON_SDMMC_PIN_CMD >= 0) &&
           (PULSEMON_SDMMC_PIN_D0 >= 0);
}
#else
static bool sd_pins_valid(void)
{
    return (PULSEMON_SD_PIN_MOSI >= 0) &&
           (PULSEMON_SD_PIN_MISO >= 0) &&
           (PULSEMON_SD_PIN_SCLK >= 0) &&
           (PULSEMON_SD_PIN_CS >= 0);
}
#endif

bool sd_storage_is_mounted(void)
{
    return s_mounted;
}

esp_err_t sd_storage_ensure_mounted(void)
{
    if (s_mounted) {
        return ESP_OK;
    }

    if (!sd_pins_valid()) {
#if PULSEMON_SD_USE_SDMMC
        ESP_LOGE(TAG, "sdmmc pins not configured (CLK=%d CMD=%d D0=%d)",
                 PULSEMON_SDMMC_PIN_CLK,
                 PULSEMON_SDMMC_PIN_CMD,
                 PULSEMON_SDMMC_PIN_D0);
#else
        ESP_LOGE(TAG, "sd pins not configured (MOSI=%d MISO=%d SCLK=%d CS=%d)",
                 PULSEMON_SD_PIN_MOSI,
                 PULSEMON_SD_PIN_MISO,
                 PULSEMON_SD_PIN_SCLK,
                 PULSEMON_SD_PIN_CS);
#endif
        return ESP_ERR_INVALID_STATE;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };

#if PULSEMON_SD_USE_SDMMC
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.clk = PULSEMON_SDMMC_PIN_CLK;
    slot_config.cmd = PULSEMON_SDMMC_PIN_CMD;
    slot_config.d0 = PULSEMON_SDMMC_PIN_D0;

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(PULSEMON_SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "sdmmc mount failed (filesystem mount)");
        } else {
            ESP_LOGE(TAG, "sdmmc mount failed: %s", esp_err_to_name(ret));
        }
        return ret;
    }
#else
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = PULSEMON_SD_SPI_HOST;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PULSEMON_SD_PIN_MOSI,
        .miso_io_num = PULSEMON_SD_PIN_MISO,
        .sclk_io_num = PULSEMON_SD_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }
    if (ret == ESP_OK) {
        s_bus_initialized = true;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.host_id = host.slot;
    slot_config.gpio_cs = PULSEMON_SD_PIN_CS;

    ret = esp_vfs_fat_sdspi_mount(PULSEMON_SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "sd mount failed (filesystem mount)");
        } else {
            ESP_LOGE(TAG, "sd mount failed: %s", esp_err_to_name(ret));
        }
        if (s_bus_initialized) {
            spi_bus_free(host.slot);
            s_bus_initialized = false;
        }
        return ret;
    }
#endif

    s_mounted = true;
    ESP_LOGI(TAG, "sd mounted on %s", PULSEMON_SD_MOUNT_POINT);
    sdmmc_card_print_info(stdout, s_card);
    return ESP_OK;
}

void sd_storage_deinit(void)
{
    if (!s_mounted) {
        return;
    }

    esp_vfs_fat_sdcard_unmount(PULSEMON_SD_MOUNT_POINT, s_card);
    s_card = NULL;
    s_mounted = false;
#if !PULSEMON_SD_USE_SDMMC
    if (s_bus_initialized) {
        spi_bus_free(PULSEMON_SD_SPI_HOST);
        s_bus_initialized = false;
    }
#endif
    ESP_LOGI(TAG, "sd unmounted from %s", PULSEMON_SD_MOUNT_POINT);
}
