/*
 * FlashDB TSDB sample for Arduino (ESP32)
 *
 * Append / query / set status of time series logs (TSL).
 *
 * Original sample: https://github.com/armink/FlashDB (samples/tsdb_sample.c)
 */

#include <flashdb.h>
#include <fal.h>
#include <string.h>
#include <time.h>

#define FDB_LOG_TAG "[sample][tsdb]"

struct env_status {
    int temp;
    int humi;
};

/* TSDB object */
static struct fdb_tsdb tsdb = { 0 };
/* counts for simulated timestamp */
static int counts = 0;

static bool query_cb(fdb_tsl_t tsl, void *arg);
static bool query_by_time_cb(fdb_tsl_t tsl, void *arg);
static bool set_status_cb(fdb_tsl_t tsl, void *arg);

static fdb_time_t get_time(void)
{
    /* Using the counts instead of timestamp.
     * Please change this function to return RTC time.
     */
    return ++counts;
}

void tsdb_sample(fdb_tsdb_t db)
{
    struct fdb_blob blob;

    FDB_INFO("==================== tsdb_sample ====================\n");

    { /* APPEND new TSL (time series log) */
        struct env_status status;

        /* append new log to TSDB */
        status.temp = 36;
        status.humi = 85;
        fdb_tsl_append(db, fdb_blob_make(&blob, &status, sizeof(status)));
        FDB_INFO("append the new status.temp (%d) and status.humi (%d)\n", status.temp, status.humi);

        status.temp = 38;
        status.humi = 90;
        fdb_tsl_append(db, fdb_blob_make(&blob, &status, sizeof(status)));
        FDB_INFO("append the new status.temp (%d) and status.humi (%d)\n", status.temp, status.humi);
    }

    { /* QUERY the TSDB */
        /* query all TSL in TSDB by iterator */
        fdb_tsl_iter(db, query_cb, db);
    }

    { /* QUERY the TSDB by time */
        /* prepare query time (from 1970-01-01 00:00:00 to 2020-05-05 00:00:00) */
        /* NOTE: C++ requires designators in struct tm declaration order (sec, min, hour, mday, mon, year) */
        struct tm tm_from = { .tm_sec = 0, .tm_min = 0, .tm_hour = 0, .tm_mday = 1, .tm_mon = 0, .tm_year = 1970 - 1900 };
        struct tm tm_to = { .tm_sec = 0, .tm_min = 0, .tm_hour = 0, .tm_mday = 5, .tm_mon = 4, .tm_year = 2020 - 1900 };
        time_t from_time = mktime(&tm_from), to_time = mktime(&tm_to);
        size_t count;
        /* query all TSL in TSDB by time */
        fdb_tsl_iter_by_time(db, from_time, to_time, query_by_time_cb, db);
        /* query all FDB_TSL_WRITE status TSL's count in TSDB by time */
        count = fdb_tsl_query_count(db, from_time, to_time, FDB_TSL_WRITE);
        FDB_INFO("query count is: %u\n", (unsigned int) count);
    }

    { /* SET the TSL status */
        /* Change the TSL status by iterator or time iterator
         * set_status_cb: the change operation will in this callback
         *
         * NOTE: The actions to modify the state must be in order.
         *       like: FDB_TSL_WRITE -> FDB_TSL_USER_STATUS1 -> FDB_TSL_DELETED -> FDB_TSL_USER_STATUS2
         *       The intermediate states can also be ignored.
         *       such as: FDB_TSL_WRITE -> FDB_TSL_DELETED
         */
        fdb_tsl_iter(db, set_status_cb, db);
    }

    FDB_INFO("=======================================================\n");
}

static bool query_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct env_status status;
    fdb_tsdb_t db = (fdb_tsdb_t) arg;

    fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &status, sizeof(status))));
    FDB_INFO("[query_cb] queried a TSL: time: %d, temp: %d, humi: %d\n", (int) tsl->time, status.temp, status.humi);

    return false;
}

static bool query_by_time_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct env_status status;
    fdb_tsdb_t db = (fdb_tsdb_t) arg;

    fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &status, sizeof(status))));
    FDB_INFO("[query_by_time_cb] queried a TSL: time: %d, temp: %d, humi: %d\n", (int) tsl->time, status.temp,
            status.humi);

    return false;
}

static bool set_status_cb(fdb_tsl_t tsl, void *arg)
{
    fdb_tsdb_t db = (fdb_tsdb_t) arg;

    FDB_INFO("set the TSL (time %d) status from %d to %d\n", (int) tsl->time, tsl->status, FDB_TSL_USER_STATUS1);
    fdb_tsl_set_status(db, tsl, FDB_TSL_USER_STATUS1);

    return false;
}

void setup()
{
    fdb_err_t result;

    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println("FlashDB TSDB sample");

    /* FAL initialization */
    if (fal_init() <= 0) {
        Serial.println("FAL init failed! Check the flash partition config.");
        while (1) {
            ;
        }
    }

    /* Time series database initialization
     *
     *       &tsdb: database object
     *       "log": database name
     * "fdb_tsdb1": The flash partition name base on FAL (defined in fal_cfg.h)
     *    get_time: The get current timestamp function.
     *         128: maximum length of each log
     *        NULL: The user data if you need, now is empty.
     */
    result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb1", get_time, 128, NULL);
    /* read last saved time for simulated timestamp */
    fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &counts);
    if (result != FDB_NO_ERR) {
        Serial.printf("TSDB init failed, error: %d\n", result);
        while (1) {
            ;
        }
    }

    tsdb_sample(&tsdb);
}

void loop()
{
}
