#include "pulsemon_diag.h"

#include <stddef.h>
#include <stdint.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

#ifndef PULSEMON_DEBUG
#define PULSEMON_DEBUG 0
#endif

static const char *TAG = "pulsemon_diag";

#if PULSEMON_DEBUG
static void pulsemon_diag_alloc_failed(size_t requested_size, uint32_t caps, const char *function_name)
{
    ets_printf("pulsemon_diag: alloc_fail function=%s bytes=%u caps=0x%08x\n",
               function_name != NULL ? function_name : "?",
               (unsigned)requested_size,
               (unsigned)caps);
}
#endif

esp_err_t pulsemon_diag_init(void)
{
#if PULSEMON_DEBUG
    return heap_caps_register_failed_alloc_callback(pulsemon_diag_alloc_failed);
#else
    return ESP_OK;
#endif
}

void pulsemon_diag_heap(const char *owner, const char *stage)
{
#if PULSEMON_DEBUG
    ESP_LOGI(TAG,
             "heap owner=%s stage=%s free_8bit=%u largest_8bit=%u internal=%u largest_internal=%u min_internal=%u dma=%u largest_dma=%u min_dma=%u spiram=%u largest_spiram=%u min_spiram=%u",
             owner != NULL ? owner : "?",
             stage != NULL ? stage : "?",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_DMA),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DMA),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_DMA),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));
#else
    (void)owner;
    (void)stage;
#endif
}

void pulsemon_diag_stack(const char *owner, const char *stage)
{
#if PULSEMON_DEBUG
    UBaseType_t high_water = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG,
             "stack owner=%s stage=%s high_water=%u",
             owner != NULL ? owner : "?",
             stage != NULL ? stage : "?",
             (unsigned)high_water);
#else
    (void)owner;
    (void)stage;
#endif
}
