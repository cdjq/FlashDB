/*
 * FlashDB KVDB blob type sample for Arduino (ESP32)
 *
 * Create / get / change / delete a blob type KV.
 *
 * Original sample: https://github.com/armink/FlashDB (samples/kvdb_type_blob_sample.c)
 */

#include <flashdb.h>
#include <fal.h>

#define FDB_LOG_TAG "[sample][kvdb][blob]"

/* KVDB object */
static struct fdb_kvdb kvdb = { 0 };

void kvdb_type_blob_sample(fdb_kvdb_t db)
{
    struct fdb_blob blob;

    FDB_INFO("==================== kvdb_type_blob_sample ====================\n");

    { /* CREATE new Key-Value */
        int temp_data = 36;

        /* It will create new KV node when "temp" KV not in database.
         * fdb_blob_make: It's a blob make function, and it will return the blob when make finish.
         */
        fdb_kv_set_blob(db, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
        FDB_INFO("create the 'temp' blob KV, value is: %d\n", temp_data);
    }

    { /* GET the KV value */
        int temp_data = 0;

        /* get the "temp" KV value */
        fdb_kv_get_blob(db, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
        /* the blob.saved.len is more than 0 when get the value successful */
        if (blob.saved.len > 0) {
            FDB_INFO("get the 'temp' value is: %d\n", temp_data);
        }
    }

    { /* CHANGE the KV value */
        int temp_data = 38;

        /* change the "temp" KV's value to 38 */
        fdb_kv_set_blob(db, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
        FDB_INFO("set 'temp' value to %d\n", temp_data);
    }

    { /* DELETE the KV by name */
        fdb_kv_del(db, "temp");
        FDB_INFO("delete the 'temp' finish\n");
    }

    FDB_INFO("===============================================================\n");
}

void setup()
{
    fdb_err_t result;

    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println("FlashDB KVDB blob type sample");

    /* FAL initialization */
    if (fal_init() <= 0) {
        Serial.println("FAL init failed! Check the flash partition config.");
        while (1) {
            ;
        }
    }

    /* Key-Value database initialization */
    result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", NULL, NULL);
    if (result != FDB_NO_ERR) {
        Serial.printf("KVDB init failed, error: %d\n", result);
        while (1) {
            ;
        }
    }

    kvdb_type_blob_sample(&kvdb);
}

void loop()
{
}
