/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FAL configuration for Arduino (ESP32).
 *
 * The whole database area lives inside ONE ESP32 flash partition
 * (see port/fal_flash_esp32_port.c). It is split into two FAL
 * partitions: fdb_kvdb1 for the key-value database and fdb_tsdb1
 * for the time series database.
 *
 * If you resize the partitions, keep each one a multiple of 4 KB
 * (the flash erase sector size) and make sure their total size
 * (NOR_FLASH_DEV_SIZE) fits into the selected ESP32 partition.
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

/* #define FAL_DEBUG      1  */
#define FAL_PART_HAS_TABLE_CFG

#define NOR_FLASH_DEV_NAME             "norflash0"

/* FAL partition sizes inside the ESP32 flash partition */
#define FDB_KVDB_PART_SIZE             (32 * 1024)   /* 32 KB for KVDB */
#define FDB_TSDB_PART_SIZE             (32 * 1024)   /* 32 KB for TSDB */
#define NOR_FLASH_DEV_SIZE             (FDB_KVDB_PART_SIZE + FDB_TSDB_PART_SIZE)

/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev nor_flash0;

/* flash device table */
#define FAL_FLASH_DEV_TABLE \
{                           \
    &nor_flash0,            \
}

/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FAL_PART_TABLE                                                                                 \
{                                                                                                      \
    {FAL_PART_MAGIC_WORD, "fdb_kvdb1", NOR_FLASH_DEV_NAME, 0,                   FDB_KVDB_PART_SIZE, 0}, \
    {FAL_PART_MAGIC_WORD, "fdb_tsdb1", NOR_FLASH_DEV_NAME, FDB_KVDB_PART_SIZE,  FDB_TSDB_PART_SIZE, 0}, \
}
#endif /* FAL_PART_HAS_TABLE_CFG */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set the ESP32 flash partition label used by FlashDB (e.g. "flashdb").
 * Optional: call BEFORE fal_init(). When not set, FlashDB auto detects
 * a "flashdb", "spiffs" or the first SPIFFS data partition.
 * Pass NULL to restore auto detection.
 */
void fdb_esp32_set_partition_label(const char *label);

#ifdef __cplusplus
}
#endif

#endif /* _FAL_CFG_H_ */
