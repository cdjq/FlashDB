# FlashDB for Arduino (ESP32)

[FlashDB](https://github.com/armink/FlashDB) 是一款超轻量级的嵌入式数据库，支持两种存储模式：

- **KVDB**：键值数据库，掉电保护、磨损均衡，适合存储产品参数、用户配置等
- **TSDB**：时序数据库，按时间顺序存储数据（如传感器记录、运行日志）

本库将其移植到 Arduino，**目前仅支持 ESP32 系列主板**（以 ESP32-S3 验证），
直接使用内部 Flash 的一个分区作为存储后端，无需文件系统。

## 使用方法

```cpp
#include <flashdb.h>
#include <fal.h>

struct fdb_kvdb kvdb = {0};

void setup() {
    Serial.begin(115200);
    fal_init();                                  // 初始化 Flash 抽象层
    fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", NULL, NULL);
    fdb_kv_set(&kvdb, "key", "value");
    Serial.println(fdb_kv_get(&kvdb, "key"));
}
```

完整用法见 `examples` 目录下的 4 个示例。

## Flash 分区说明（重要）

FlashDB 需要 ESP32 分区表中的一个 **data 类型分区**（至少 64KB，可在
`src/fal_cfg.h` 中修改大小）。默认按以下顺序自动查找分区：

1. 名为 `flashdb` 的分区（推荐，自定义分区表中添加）
2. 名为 `spiffs` 的分区（Arduino ESP32 默认分区表自带，**此时不要再挂载 SPIFFS**）
3. 第一个 subtype 为 SPIFFS 的 data 分区

也可以在 `fal_init()` 之前调用指定分区名：

```cpp
fdb_esp32_set_partition_label("flashdb");
```

### 自定义分区表

如果主板默认分区表中没有可用分区（如某些 4MB 单 APP 分区表），在 sketch 目录下
添加 `partitions.csv` 即可覆盖（本平台已支持自动拷贝），例如：

```csv
# Name,   Type, SubType, Offset,  Size,     Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 2M,
flashdb,  data, 0x40,    ,        256K,
```

## 目录结构

- `src/`：FlashDB 内核（`fdb_*.c`）+ FAL 抽象层（`fal*.c`），均为上游原版源码
- `src/fdb_cfg.h`：FlashDB 功能配置（KVDB/TSDB 开关等）
- `src/fal_cfg.h`：FAL 分区布局配置（KVDB/TSDB 各自大小）
- `src/port/fal_flash_esp32_port.c`：ESP32 分区移植层（esp_partition API + FreeRTOS 互斥锁）

## 参考

- 上游仓库：https://github.com/armink/FlashDB
- 官方文档：https://armink.github.io/FlashDB/#/zh/
