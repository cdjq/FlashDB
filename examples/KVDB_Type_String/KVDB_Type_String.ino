/*
 * FlashDB KVDB string type sample for Arduino (ESP32)
 *
 * Create / get / change / delete a string type KV.
 *
 * Original sample: https://github.com/armink/FlashDB (samples/kvdb_type_string_sample.c)
 */

#include <flashdb.h>
#include <fal.h>

#define FDB_LOG_TAG "[sample][kvdb][string]"
#include <string.h>

/* KVDB object */
static struct fdb_kvdb kvdb = { 0 };

void kvdb_type_string_sample(fdb_kvdb_t db)
{
    FDB_INFO("==================== kvdb_type_string_sample ====================\n");

    { /* CREATE new Key-Value */
        char temp_data[10] = "36C";

        /* It will create new KV node when "temp" KV not in database. */
        fdb_kv_set(db, "temp", temp_data);
        FDB_INFO("create the 'temp' string KV, value is: %s\n", temp_data);
    }

    { /* GET the KV value */
        char *return_value, temp_data[10] = { 0 };

        /* Get the "temp" KV value.
         * NOTE: The return value saved in fdb_kv_get's buffer. Please copy away as soon as possible.
         */
        return_value = fdb_kv_get(db, "temp");
        /* the return value is NULL when get the value failed */
        if (return_value != NULL) {
            strncpy(temp_data, return_value, sizeof(temp_data));
            FDB_INFO("get the 'temp' value is: %s\n", temp_data);
        }
    }

    { /* CHANGE the KV value */
        char temp_data[10] = "38C";

        /* change the "temp" KV's value to "38C" */
        fdb_kv_set(db, "temp", temp_data);
        FDB_INFO("set 'temp' value to %s\n", temp_data);
    }

    { /* DELETE the KV by name */
        fdb_kv_del(db, "temp");
        FDB_INFO("delete the 'temp' finish\n");
    }

    FDB_INFO("=================================================================\n");
}

void setup()
{
    fdb_err_t result;

    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println("FlashDB KVDB string type sample");

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

    kvdb_type_string_sample(&kvdb);
}

void loop()
{
}
