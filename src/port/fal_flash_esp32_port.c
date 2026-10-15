/*
 * Copyright (c) 2022, kaans, <https://github.com/kaans>
 * Copyright (c) 2024, Arduino port
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FAL flash device port for ESP32 (Arduino), based on esp_partition API.
 *
 * FlashDB uses ONE ESP32 flash partition (label selected at runtime,
 * "flashdb" / "spiffs" by default) and splits it into the FAL partitions
 * defined in fal_cfg.h (fdb_kvdb1 + fdb_tsdb1).
 */

#include <fal.h>

#if defined(ESP32)

#include <string.h>
#include <esp_partition.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define FLASH_ERASE_MIN_SIZE    (4 * 1024)

static SemaphoreHandle_t s_lock = NULL;
static const esp_partition_t *s_partition = NULL;

/* partition label can be changed before fal_init() via fdb_esp32_set_partition_label() */
static char s_partition_label[FAL_DEV_NAME_MAX] =
#ifdef FDB_ESP32_PARTITION_LABEL
    FDB_ESP32_PARTITION_LABEL;
#else
    "";
#endif

#define LOCK()                                   \
    do {                                         \
        if (s_lock) {                            \
            xSemaphoreTake(s_lock, portMAX_DELAY); \
        }                                        \
    } while (0)

#define UNLOCK()                \
    do {                        \
        if (s_lock) {           \
            xSemaphoreGive(s_lock); \
        }                       \
    } while (0)

/**
 * Set the ESP32 flash partition label used by FlashDB.
 * Call it BEFORE fal_init(). Pass NULL to restore auto detection.
 */
void fdb_esp32_set_partition_label(const char *label)
{
    if (label) {
        strncpy(s_partition_label, label, sizeof(s_partition_label) - 1);
        s_partition_label[sizeof(s_partition_label) - 1] = '\0';
    } else {
        s_partition_label[0] = '\0';
    }
}

static const esp_partition_t *find_flashdb_partition(void)
{
    const esp_partition_t *partition = NULL;

    /* 1. label set by the user or by FDB_ESP32_PARTITION_LABEL */
    if (s_partition_label[0] != '\0') {
        partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, s_partition_label);
        if (partition == NULL) {
            log_e("Flash partition '%s' NOT found. Check your partition table.", s_partition_label);
        }
        return partition;
    }

    /* 2. auto detection: a dedicated "flashdb" partition first ... */
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "flashdb");
    if (partition) {
        return partition;
    }
    /* ... then the common "spiffs" partition ... */
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "spiffs");
    if (partition) {
        log_i("Using 'spiffs' partition for FlashDB. Do NOT mount SPIFFS on it at the same time.");
        return partition;
    }
    /* ... finally the first data partition with spiffs subtype */
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
    if (partition) {
        log_i("Using '%s' partition (first SPIFFS data partition) for FlashDB.", partition->label);
    } else {
        log_e("No usable flash partition found for FlashDB. Add a 'flashdb' data partition to your partition table.");
    }
    return partition;
}

static int init(void)
{
    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateMutex();
        assert(s_lock != NULL);
    }

    if (s_partition == NULL) {
        s_partition = find_flashdb_partition();
        if (s_partition == NULL) {
            /* NOTE: FAL ignores init()'s return value, all ops below are NULL-safe */
            return -1;
        }
        if (s_partition->size < NOR_FLASH_DEV_SIZE) {
            log_e("Partition '%s' is too small: %u bytes, need at least %u bytes.",
                    s_partition->label, (unsigned int) s_partition->size, (unsigned int) NOR_FLASH_DEV_SIZE);
            s_partition = NULL;
            return -1;
        }
        log_i("FlashDB uses partition '%s' (0x%x bytes at 0x%x).",
                s_partition->label, (unsigned int) s_partition->size, (unsigned int) s_partition->address);
    }

    return 1;
}

static int read(long offset, uint8_t *buf, size_t size)
{
    esp_err_t ret;

    if (s_partition == NULL) {
        return -1;
    }
    LOCK();
    ret = esp_partition_read(s_partition, offset, buf, size);
    UNLOCK();

    return ret;
}

static int write(long offset, const uint8_t *buf, size_t size)
{
    esp_err_t ret;

    if (s_partition == NULL) {
        return -1;
    }
    LOCK();
    ret = esp_partition_write(s_partition, offset, buf, size);
    UNLOCK();

    return ret;
}

static int erase(long offset, size_t size)
{
    esp_err_t ret;
    int32_t erase_size = ((size - 1) / FLASH_ERASE_MIN_SIZE) + 1;

    if (s_partition == NULL) {
        return -1;
    }
    LOCK();
    ret = esp_partition_erase_range(s_partition, offset, erase_size * FLASH_ERASE_MIN_SIZE);
    UNLOCK();

    return ret;
}

const struct fal_flash_dev nor_flash0 =
{
    .name       = NOR_FLASH_DEV_NAME,
    .addr       = 0x0,                      /* address is relative to beginning of the ESP32 partition */
    .len        = NOR_FLASH_DEV_SIZE,       /* usable size configured in fal_cfg.h */
    .blk_size   = FLASH_ERASE_MIN_SIZE,     /* must be 4096 bytes */
    .ops        = {init, read, write, erase},
    .write_gran = 1,                        /* 1 bit write granularity */
};

#endif /* ESP32 */
