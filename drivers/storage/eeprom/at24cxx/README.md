# AT24Cxx EEPROM 通用驱动

基于 `soft_i2c_simple` 层实现，与项目中的 OLED 驱动共用同一套 I2C 抽象层。

## 文件结构

```
eeprom/at24cxx/
├── board_i2c_pins.h      # 引脚集中配置，换板只改这里
├── at24cxx.h             # 驱动头文件
├── at24cxx.c             # 驱动实现
├── README.md
└── test/
    └── at24cxx_test.c    # 验证测试程序
```

## 依赖

- `soft_i2c_simple.h` — 软件 I2C 简单层（和 OLED 用同一个）
- `board_i2c_pins.h` — 引脚集中配置（自动包含）
- `delay.h` — 延时模块
- `oled.h` — 仅测试文件需要

## ★ 移植时需要的改动？

**只改 `board_i2c_pins.h` 中的 4 行宏：**

```c
#define I2C_SCL_PORT  GPIOB       // 改 SCL 端口
#define I2C_SCL_PIN   GPIO_PIN_6  // 改 SCL 引脚
#define I2C_SDA_PORT  GPIOB       // 改 SDA 端口
#define I2C_SDA_PIN   GPIO_PIN_7  // 改 SDA 引脚
```

所有引用这个总线的外设（EEPROM、OLED 等）自动适配。

## 快速开始

### 1. 初始化 I2C 总线

```c
#include "board_i2c_pins.h"

SoftI2C_Obj i2c_shared = I2C_PINS_INITIALIZER;  // 从引脚配置自动生成

// 在 main 初始化中调用
SoftI2C_Init(&i2c_shared);
```

### 2. 初始化 EEPROM 驱动

```c
AT24Cxx_Config_t cfg = {
    .i2c       = &i2c_shared,      // 共享同一总线
    .dev_addr  = 0x50,             // A0=A1=A2=GND
    .page_size = AT24C02_PAGE_SIZE,// 8 字节
    .addr_size = AT24C_ADDR_8BIT,  // 8位地址
    .capacity  = 256,              // AT24C02 = 256 字节
};
AT24Cxx_Init(&cfg);
```

### 3. 读写操作

```c
// 单字节
AT24Cxx_WriteByte(0x00, 0xA5);
uint8_t val;
AT24Cxx_ReadByte(0x00, &val);  // val = 0xA5

// 页写入
uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
AT24Cxx_WritePage(0x00, data, 8);

// 连续写入（自动跨页）
uint8_t big[24];
AT24Cxx_Write(0x00, big, 24);

// 连续读取
AT24Cxx_Read(0x00, big, 24);
```

### 4. OLED 共用同一总线

在 `soft_i2c_simple.c` 中，把 `i2c_oled` 定义改为：

```c
#include "board_i2c_pins.h"

SoftI2C_Obj i2c_oled = I2C_PINS_INITIALIZER;  // 和 i2c_shared 同引脚
```

或者直接用 `extern SoftI2C_Obj i2c_shared;` 替代 `i2c_oled`。

## 型号配置速查表

| 型号      | 容量   | 页大小 | 地址宽度 | 地址  |
|-----------|--------|--------|----------|-------|
| AT24C01   | 128 B  | 8      | 8-bit    | 0x50  |
| AT24C02   | 256 B  | 8      | 8-bit    | 0x50  |
| AT24C04   | 512 B  | 16     | 8-bit    | 0x50¹ |
| AT24C08   | 1 KB   | 16     | 8-bit    | 0x50¹ |
| AT24C16   | 2 KB   | 16     | 8-bit    | 0x50¹ |
| AT24C32   | 4 KB   | 32     | 16-bit   | 0x50  |
| AT24C64   | 8 KB   | 32     | 16-bit   | 0x50  |
| AT24C128  | 16 KB  | 64     | 16-bit   | 0x50  |
| AT24C256  | 32 KB  | 64     | 16-bit   | 0x50  |

¹ 较大容量低端型号 (04/08/16) 中 A0/A1/A2 引脚用作块选，需在 `dev_addr` 中设置对应的块地址。

## 验证测试

调用 `AT24Cxx_EEPROMM_Test()` 运行全部 5 项测试，结果通过 OLED 显示。
T5 为掉电保存验证，需手动断电再上电。
