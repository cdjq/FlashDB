/*
 * FlashDB KVDB basic sample for Arduino (ESP32)
 *
 * Get and show the current boot count from the key-value database.
 * Reboot the board and watch the counter increase.
 *
 * Original sample: https://github.com/armink/FlashDB (samples/kvdb_basic_sample.c)
 */

#include <flashdb.h>
#include <fal.h>

#define FDB_LOG_TAG "[sample][kvdb][basic]"

static uint32_t boot_count = 0;

/* default KV nodes, they are added to the database on first initialize */
static struct fdb_default_kv_node default_kv_table[] = {
    { "boot_count", &boot_count, sizeof(boot_count) }, /* int type KV */
};

/* KVDB object */
static struct fdb_kvdb kvdb = { 0 };

void kvdb_basic_sample(fdb_kvdb_t db)
{
    struct fdb_blob blob;
    int count = 0;

    FDB_INFO("==================== kvdb_basic_sample ====================\n");

    { /* GET the KV value */
        /* get the "boot_count" KV value */
        fdb_kv_get_blob(db, "boot_count", fdb_blob_make(&blob, &count, sizeof(count)));
        /* the blob.saved.len is more than 0 when get the value successful */
        if (blob.saved.len > 0) {
            FDB_INFO("get the 'boot_count' value is %d\n", count);
        } else {
            FDB_INFO("get the 'boot_count' failed\n");
        }
    }

    { /* CHANGE the KV value */
        /* increase the boot count */
        count++;
        /* change the "boot_count" KV's value */
        fdb_kv_set_blob(db, "boot_count", fdb_blob_make(&blob, &count, sizeof(count)));
        FDB_INFO("set the 'boot_count' value to %d\n", count);
    }

    FDB_INFO("===========================================================\n");
}

void setup()
{
    fdb_err_t result;
    struct fdb_default_kv default_kv;

    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println("FlashDB KVDB basic sample");

    /* FAL initialization */
    if (fal_init() <= 0) {
        Serial.println("FAL init failed! Check the flash partition config.");
        while (1) {
            ;
        }
    }

    default_kv.kvs = default_kv_table;
    default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);

    /* Key-Value database initialization
     *
     *       &kvdb: database object
     *       "env": database name
     * "fdb_kvdb1": The flash partition name base on FAL (defined in fal_cfg.h)
     * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
     *        NULL: The user data if you need, now is empty.
     */
    result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);
    if (result != FDB_NO_ERR) {
        Serial.printf("KVDB init failed, error: %d\n", result);
        while (1) {
            ;
        }
    }

    kvdb_basic_sample(&kvdb);
}

void loop()
{
}
