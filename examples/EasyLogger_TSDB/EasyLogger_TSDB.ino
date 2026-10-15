/*
 * FlashDB TSDB + EasyLogger demo for Arduino (ESP32)
 *
 * Redirects EasyLogger output into FlashDB's time series database (TSDB):
 * every log line produced by LOG_ERROR/LOG_WARNING/LOG_INFO/... is appended
 * to the TSDB (and mirrored to Serial). Then all saved log lines are queried
 * back from flash and printed.
 *
 * Log lines survive reboots: reset or power-cycle the board and run again,
 * the previously saved logs are still queried out of the TSDB.
 */

#include <flashdb.h>
#include <fal.h>

/* ------------------------------------------------------------------ */
/* EasyLogger output -> TSDB bridge                                    */
/* ------------------------------------------------------------------ */

/**
 * A Print that forwards everything to Serial AND saves each finished
 * log line (terminated by '\n') as one TSL record into the TSDB.
 */
class TsdbPrint : public Print {
  public:
    void bind(fdb_tsdb_t db) { _db = db; }

    virtual size_t write(uint8_t c) {
      Serial.write(c); /* mirror to Serial for live view */
      if (c == '\n') { /* line finished -> save to TSDB */
        struct fdb_blob blob;
        /* strip trailing '\r' */
        while (_len > 0 && (_buf[_len - 1] == '\r' || _buf[_len - 1] == '\n')) {
          _len--;
        }
        _buf[_len] = '\0';
        if (_len > 0 && _db != NULL) {
          fdb_tsl_append(_db, fdb_blob_make(&blob, _buf, _len));
        }
        _len = 0;
        return 1;
      }
      if (_len < sizeof(_buf) - 1) {
        _buf[_len++] = (char) c;
      } /* else: line too long, truncate */
      return 1;
    }

  private:
    fdb_tsdb_t _db = NULL;
    char _buf[96];
    size_t _len = 0;
};

static TsdbPrint tsdbLog;

/* Route EasyLogger output to our Print (must be defined BEFORE the include) */
#define LOG_OUTPUT tsdbLog
#include <EasyLogger.h>

/* ------------------------------------------------------------------ */
/* TSDB setup                                                          */
/* ------------------------------------------------------------------ */

/* TSDB object */
static struct fdb_tsdb tsdb = { 0 };
/* timestamp base, restored from flash so time keeps growing across reboots */
static fdb_time_t boot_base = 0;

static fdb_time_t get_time(void)
{
    static fdb_time_t last = 0;
    fdb_time_t now = boot_base + millis() / 1000; /* seconds since first boot */
    if (now <= last) {
        now = last + 1; /* keep timestamps strictly increasing */
    }
    last = now;
    return now;
}

/* iterator callback: print one saved log line from the TSDB */
static bool print_log_cb(fdb_tsl_t tsl, void *arg)
{
    char buf[112];
    struct fdb_blob blob;
    size_t len;

    len = fdb_blob_read((fdb_db_t) arg, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, buf, sizeof(buf) - 1)));
    buf[len] = '\0';
    Serial.printf("  [TSDB] time=%5u s | %s\n", (unsigned int) tsl->time, buf);

    return false; /* continue iteration */
}

/* query and print all log lines saved in the TSDB */
static void dump_saved_logs(const char *title)
{
    size_t count = fdb_tsl_query_count(&tsdb, 0, 0x7FFFFFFF, FDB_TSL_WRITE);

    Serial.printf("--- %s (%u record(s)) ---\n", title, (unsigned int) count);
    fdb_tsl_iter(&tsdb, print_log_cb, &tsdb);
    Serial.println("------------------------------------------------");
}

/* ------------------------------------------------------------------ */

void setup()
{
    fdb_err_t result;
    fdb_time_t last_time = 0;

    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println("\nFlashDB TSDB + EasyLogger demo");

    /* FAL initialization */
    if (fal_init() <= 0) {
        Serial.println("FAL init failed! Check the flash partition config.");
        while (1) {
            ;
        }
    }

    /* Time series database initialization
     * "log": database name, "fdb_tsdb1": FAL partition name (see fal_cfg.h),
     * 96: maximum length of each log line.
     */
    result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb1", get_time, 96, NULL);
    if (result != FDB_NO_ERR) {
        Serial.printf("TSDB init failed, error: %d\n", result);
        while (1) {
            ;
        }
    }
    /* continue timestamps where the last boot stopped */
    fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &last_time);
    boot_base = last_time;

    /* connect EasyLogger output to the TSDB */
    tsdbLog.bind(&tsdb);

    /* 1. show the log lines saved during PREVIOUS boots (still in flash) */
    dump_saved_logs("logs restored from flash");

    /* 2. produce some fresh logs - each line goes to Serial AND TSDB */
    LOG_EMERGENCY("SYS", "system boot #" << (boot_base > 0 ? "N (reboot)" : "1 (first run)"));
    LOG_ERROR("SENSOR", "temperature read failed, retrying");
    LOG_WARNING("SENSOR", "temperature too high: " << 42.5 << " C");
    LOG_INFO("NET", "wifi connected, rssi=" << -55 << " dBm");
    LOG_DEBUG("MAIN", "setup() finished");

    /* 3. query everything now stored in the TSDB */
    dump_saved_logs("all logs in TSDB after this boot");

    Serial.println("Now logging a heartbeat every 10 seconds...");
}

void loop()
{
    static uint32_t last = 0;

    if (millis() - last >= 10000) {
        last = millis();
        /* each heartbeat is also appended to the TSDB */
        LOG_NOTICE("MAIN", "heartbeat, uptime=" << (millis() / 1000) << " s, free heap=" << ESP.getFreeHeap());
    }
}
