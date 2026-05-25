/**
 * @file    at24cxx_test.c
 * @brief   AT24Cxx EEPROM 读写验证测试程序
 * @author  XuChengShuo
 * @date    2026-05-25
 *
 * @note    基于 soft_i2c_simple 层，与 OLED 共用同一 I2C 总线
 *          引脚配置统一在 board_i2c_pins.h 中，只改那里
 *
 *          测试流程:
 *          1. I2C 总线扫描 → 确认 EEPROM 器件存在
 *          2. 单字节写入/读取验证
 *          3. 页写入验证
 *          4. 跨页连续写入验证
 *          5. 字符串读写验证
 *          6. 掉电保存验证 (首次写入签名，断电重启后读取)
 *
 *          调试方法 (参考 OLED 驱动):
 *          - 用 I2C_ScanBus 扫描总线确认器件地址
 *          - 支持逐字节 ACK 检测定位通信故障
 *          - OLED 实时显示测试结果
 *          - 配合逻辑分析仪抓取 SCL/SDA 时序
 */

#include "at24cxx.h"
#include "soft_i2c_simple.h" /* SoftI2C_Init / I2C_ScanBus */
#include "board_i2c_pins.h"  /* I2C_PINS_INITIALIZER */
#include "oled.h"            /* OLED 显示调试信息 */
#include <string.h>

/*===========================================================================
 * 测试配置
 *===========================================================================*/

/* 根据实际 EEPROM 型号修改 */
#define TEST_DEV_ADDR     AT24C_DEFAULT_ADDR /* 0x50         */
#define TEST_PAGE_SIZE    AT24C02_PAGE_SIZE  /* 8 字节       */
#define TEST_ADDR_SIZE    AT24C_ADDR_8BIT    /* 8位地址      */
#define TEST_CAPACITY     256                /* AT24C02 容量 */
#define TEST_HELLO_ADDR   0x00               /* 测试字符串   */
#define TEST_COUNTER_ADDR 0xF0               /* 掉电计数地址 */

/* 调试开关: 无 OLED 时启用 UART printf */
// #define ENABLE_PRINTF

/*===========================================================================
 * 共享 I2C 总线实例 — ★★★ 这是唯一的引脚定义处 ★★★
 *
 * 换开发板时：只需改 board_i2c_pins.h 中的 I2C_SCL_PORT / PIN / I2C_SDA_PORT / PIN
 * 这里和 OLED 的 i2c_oled 使用同一组引脚，所以共用同一个总线对象
 *===========================================================================*/
SoftI2C_Obj i2c_shared = I2C_PINS_INITIALIZER;

/* 让 OLED 也能引用同一个总线 extern SoftI2C_Obj i2c_oled */
/* 在 soft_i2c_simple.c 中，可以把 i2c_oled 改成引用 i2c_shared，或者在 main.c 中重映射 */

/*===========================================================================
 * AT24Cxx 配置
 *===========================================================================*/
static AT24Cxx_Config_t g_eeprom_cfg = {
    .i2c       = &i2c_shared,
    .dev_addr  = TEST_DEV_ADDR,
    .page_size = TEST_PAGE_SIZE,
    .addr_size = TEST_ADDR_SIZE,
    .capacity  = TEST_CAPACITY,
};

/*===========================================================================
 * 测试辅助函数
 *===========================================================================*/

static void show_test_status(uint8_t line, const char *label, AT24Cxx_Status_t status)
{
    OLED_ShowString(line, 1, (char *) label);
    if (status == AT24CXX_OK)
    {
        OLED_ShowString(line, 13, "OK");
    }
    else
    {
        OLED_ShowHexNum(line, 13, (uint32_t) (-status), 2);
        OLED_ShowString(line, 15, "ERR");
    }
}

static bool buf_compare(const uint8_t *expected, const uint8_t *actual, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        if (expected[i] != actual[i])
        {
            OLED_ShowHexNum(4, 1, i, 4);
            OLED_ShowHexNum(4, 6, expected[i], 2);
            OLED_ShowHexNum(4, 9, actual[i], 2);
            return false;
        }
    }
    return true;
}

/*===========================================================================
 * I2C 调试函数 (参考 OLED 驱动调试方法)
 *===========================================================================*/

/**
 * @brief  检测 EEPROM 器件是否在线
 * @note   参考 OLED 驱动的 I2C_ScanBus 扫描方法
 */
static bool eeprom_detect(void)
{
    OLED_ShowString(1, 1, "Scan I2C...   ");

    /* 直接扫描目标地址 */
    SoftI2C_Start(&i2c_shared);
    SoftI2C_SendByte(&i2c_shared, (uint8_t) (TEST_DEV_ADDR << 1) | SOFT_I2C_WRITE);
    uint8_t ack = SoftI2C_WaitAck(&i2c_shared);
    SoftI2C_Stop(&i2c_shared);

    if (ack == 0) /* 收到 ACK */
    {
        OLED_ShowString(1, 1, "EEPROM @0x");
        OLED_ShowHexNum(1, 11, TEST_DEV_ADDR, 2);
        delay_ms(500);
        return true;
    }

    /* 全地址扫描 */
    uint8_t found = I2C_ScanBus(&i2c_shared);
    if (found > 0)
    {
        OLED_ShowString(1, 1, "Found dev(s)  ");
        OLED_ShowNum(1, 14, found, 2);
        delay_ms(1000);
        return true;
    }

    OLED_ShowString(1, 1, "NO EEPROM!!  ");
    return false;
}

/**
 * @brief  手动 I2C 逐字节调试 (定位通信故障)
 */
static void i2c_debug_step(void)
{
    OLED_ShowString(4, 1, "DBG:          ");

    SoftI2C_Start(&i2c_shared);
    OLED_ShowString(4, 1, "START OK      ");
    delay_ms(200);

    SoftI2C_SendByte(&i2c_shared, (uint8_t) (TEST_DEV_ADDR << 1) | SOFT_I2C_WRITE);
    if (SoftI2C_WaitAck(&i2c_shared) == 0)
    {
        OLED_ShowString(4, 1, "ADDR+ACK OK   ");
    }
    else
    {
        OLED_ShowString(4, 1, "ADDR+NACK!    ");
    }
    delay_ms(200);

    SoftI2C_Stop(&i2c_shared);
    OLED_ShowString(4, 1, "STOP OK       ");
    delay_ms(500);

    OLED_ShowHexNum(4, 12, TEST_DEV_ADDR, 2);
}

/*===========================================================================
 * 测试用例
 *===========================================================================*/

static AT24Cxx_Status_t test_single_byte(void)
{
    uint8_t written, readback;
    OLED_ShowString(2, 1, "T1: SingleByte");

    written            = 0xA5;
    AT24Cxx_Status_t s = AT24Cxx_WriteByte(0x00, written);
    if (s != AT24CXX_OK)
        return s;

    s = AT24Cxx_ReadByte(0x00, &readback);
    if (s != AT24CXX_OK)
        return s;
    if (readback != written)
        return AT24CXX_ERR_I2C;

    written = 0x5A;
    s       = AT24Cxx_WriteByte(0x01, written);
    if (s != AT24CXX_OK)
        return s;

    s = AT24Cxx_ReadByte(0x01, &readback);
    if (s != AT24CXX_OK)
        return s;
    if (readback != written)
        return AT24CXX_ERR_I2C;

    return AT24CXX_OK;
}

static AT24Cxx_Status_t test_page_write(void)
{
    uint8_t wbuf[TEST_PAGE_SIZE], rbuf[TEST_PAGE_SIZE];
    OLED_ShowString(2, 1, "T2: PageWrite  ");

    for (uint16_t i = 0; i < TEST_PAGE_SIZE; i++) wbuf[i] = (uint8_t) i;
    AT24Cxx_Status_t s = AT24Cxx_WritePage(0x00, wbuf, TEST_PAGE_SIZE);
    if (s != AT24CXX_OK)
        return s;
    s = AT24Cxx_Read(0x00, rbuf, TEST_PAGE_SIZE);
    if (s != AT24CXX_OK)
        return s;
    if (!buf_compare(wbuf, rbuf, TEST_PAGE_SIZE))
        return AT24CXX_ERR_I2C;

    for (uint16_t i = 0; i < TEST_PAGE_SIZE; i++) wbuf[i] = (uint8_t) (0x80 + i);
    s = AT24Cxx_WritePage(TEST_PAGE_SIZE, wbuf, TEST_PAGE_SIZE);
    if (s != AT24CXX_OK)
        return s;
    s = AT24Cxx_Read(TEST_PAGE_SIZE, rbuf, TEST_PAGE_SIZE);
    if (s != AT24CXX_OK)
        return s;
    if (!buf_compare(wbuf, rbuf, TEST_PAGE_SIZE))
        return AT24CXX_ERR_I2C;

    return AT24CXX_OK;
}

static AT24Cxx_Status_t test_cross_page_write(void)
{
    uint16_t total = TEST_PAGE_SIZE * 3;
    uint8_t  wbuf[64], rbuf[64];
    OLED_ShowString(2, 1, "T3: CrossPage ");

    for (uint16_t i = 0; i < total; i++) wbuf[i] = (uint8_t) (i & 0xFF);
    AT24Cxx_Status_t s = AT24Cxx_Write(0x10, wbuf, total);
    if (s != AT24CXX_OK)
        return s;
    s = AT24Cxx_Read(0x10, rbuf, total);
    if (s != AT24CXX_OK)
        return s;
    if (!buf_compare(wbuf, rbuf, total))
        return AT24CXX_ERR_I2C;

    return AT24CXX_OK;
}

static AT24Cxx_Status_t test_string_rw(void)
{
    const char *str = "Hello EEPROM!";
    uint8_t     rbuf[32];
    uint16_t    len = strlen(str) + 1;
    OLED_ShowString(2, 1, "T4: StringRW  ");

    AT24Cxx_Status_t s = AT24Cxx_Write(TEST_HELLO_ADDR, (const uint8_t *) str, len);
    if (s != AT24CXX_OK)
        return s;
    s = AT24Cxx_Read(TEST_HELLO_ADDR, rbuf, len);
    if (s != AT24CXX_OK)
        return s;
    if (memcmp(str, rbuf, len) != 0)
        return AT24CXX_ERR_I2C;

    OLED_ShowString(3, 1, "-> ");
    OLED_ShowString(3, 3, (char *) rbuf);
    return AT24CXX_OK;
}

static AT24Cxx_Status_t test_power_off_persistence(void)
{
    uint8_t sig, counter;
    OLED_ShowString(2, 1, "T5: PowerOff  ");

    AT24Cxx_Status_t s = AT24Cxx_ReadByte(TEST_COUNTER_ADDR, &sig);
    if (s != AT24CXX_OK)
        return s;
    if (sig == 0xAA)
    {
        s = AT24Cxx_ReadByte(TEST_COUNTER_ADDR + 1, &counter);
        if (s != AT24CXX_OK)
            return s;
        counter++;
        s = AT24Cxx_WriteByte(TEST_COUNTER_ADDR + 1, counter);
        if (s != AT24CXX_OK)
            return s;

        OLED_ShowString(1, 1, "Power-off OK! ");
        OLED_ShowString(4, 1, "Count:        ");
        OLED_ShowNum(4, 8, counter, 3);
    }
    else
    {
        s = AT24Cxx_WriteByte(TEST_COUNTER_ADDR, 0xAA);
        if (s != AT24CXX_OK)
            return s;
        s = AT24Cxx_WriteByte(TEST_COUNTER_ADDR + 1, 0);
        if (s != AT24CXX_OK)
            return s;

        OLED_ShowString(1, 1, "First boot!   ");
        OLED_ShowString(4, 1, "Now poweroff! ");
    }
    return AT24CXX_OK;
}

/*===========================================================================
 * 主测试入口
 *===========================================================================*/

/**
 * @brief  执行所有 EEPROM 测试
 *
 * 典型调用 (在 main 中):
 * @code
 * int main(void)
 * {
 *     HAL_Init();
 *     SystemClock_Config();
 *     delay_init();
 *     OLED_Init();
 *     MX_GPIO_Init();        // CubeMX: 配置 PB6/PB7
 *
 *     // 初始化 I2C 总线 (EEPROM + OLED 共用)
 *     SoftI2C_Init(&i2c_shared);
 *
 *     // 如果在 soft_i2c_simple.c 中 OLED 用了不同引脚，
 *     // 需确保 i2c_oled 也指向 PB6/PB7
 *
 *     AT24Cxx_EEPROMM_Test();
 *
 *     while (1);
 * }
 * @endcode
 */
void AT24Cxx_EEPROMM_Test(void)
{
    AT24Cxx_Status_t status;
    uint8_t          pass = 0, fail = 0;

    OLED_Clear();
    OLED_ShowString(1, 1, "EEPROM Test ");
    delay_ms(500);

    /* 第一步: 初始化 I2C 总线 */
    SoftI2C_Init(&i2c_shared);
    delay_ms(10);

    /* 第二步: 检测 EEPROM 器件 */
    if (!eeprom_detect())
    {
        OLED_ShowString(2, 1, "EEPROM: NONE! ");
        OLED_ShowString(3, 1, "Check wiring:");
        OLED_ShowString(4, 1, "SCL/SDA/PWR  ");
        delay_ms(2000);
        i2c_debug_step();
        delay_ms(3000);

        OLED_Clear();
        OLED_ShowString(1, 1, "Checklist:    ");
        OLED_ShowString(2, 1, "1. VCC=3.3V   ");
        OLED_ShowString(3, 1, "2. SCL/SDA OK ");
        OLED_ShowString(4, 1, "3. 4.7k pullup");
        delay_ms(5000);
        return;
    }

    /* 第三步: 初始化 EEPROM 驱动 */
    status = AT24Cxx_Init(&g_eeprom_cfg);
    if (status != AT24CXX_OK)
    {
        OLED_ShowString(2, 1, "Init FAIL!     ");
        while (1);
    }
    delay_ms(200);

    /* 第四步: 运行测试用例 */
    struct
    {
        AT24Cxx_Status_t (*func)(void);
        const char *name;
    } tests[] = {
        {test_single_byte,           "T1 SingleByte"},
        {test_page_write,            "T2 PageWrite" },
        {test_cross_page_write,      "T3 CrossPage" },
        {test_string_rw,             "T4 StringRW"  },
        {test_power_off_persistence, "T5 PowerOff"  },
    };
    uint8_t num = sizeof(tests) / sizeof(tests[0]);

    for (uint8_t i = 0; i < num; i++)
    {
        OLED_ShowString(2, 1, "                ");
        status = tests[i].func();
        show_test_status(2, tests[i].name, status);
        delay_ms(300);

        if (status == AT24CXX_OK)
            pass++;
        else
        {
            fail++;
            OLED_ShowString(2, 10, "FAIL");
            OLED_ShowHexNum(3, 1, (uint32_t) (-status), 2);
            OLED_ShowString(4, 1, "Check wiring! ");
            delay_ms(2000);
        }
        delay_ms(200);
    }

    /* 第五步: 输出总结 */
    OLED_Clear();
    OLED_ShowString(1, 1, "=== EEPROM ===");
    OLED_ShowString(2, 1, "Pass:");
    OLED_ShowNum(2, 6, pass, 2);
    OLED_ShowString(2, 9, "/");
    OLED_ShowNum(2, 10, num, 2);

    if (fail == 0)
    {
        OLED_ShowString(3, 1, "ALL TESTS PASS");
        OLED_ShowString(4, 1, "I2C: OK       ");
    }
    else
    {
        OLED_ShowString(3, 1, "Fail:");
        OLED_ShowNum(3, 6, fail, 2);
        OLED_ShowString(4, 1, "Check wiring! ");
    }
}

/*===========================================================================
 * 掉电保存验证方法
 *===========================================================================
 * 1. 烧录 → 首次运行 → 写入签名 0xAA + 计数器 0
 *    OLED 显示 "First boot!  Now poweroff!"
 * 2. 断开电源，等待 5 秒
 * 3. 重新上电 → 读到签名 0xAA → 计数器+1
 *    OLED 显示 "Power-off OK! Count: 1"
 * 4. 重复 2~3 可验证多次掉电保存
 *===========================================================================*/

/*===========================================================================
 * 接线检查清单
 *===========================================================================
 * EEPROM     STM32F103
 * VCC   →    3.3V
 * GND   →    GND
 * SCL   →    PB6  ← board_i2c_pins.h 中定义
 * SDA   →    PB7  ← board_i2c_pins.h 中定义
 * WP    →    GND (写保护必须接地)
 * A0~A2 →    GND (地址 0x50)
 *
 * I2C 通信排查:
 * 1. 上拉电阻: SCL/SDA 各 4.7kΩ → 3.3V
 * 2. WP 必须接 GND，否则写不进去
 * 3. 用 I2C_ScanBus 确认器件应答
 * 4. OLED 不显示时检查 OLED 引脚是否也在 PB6/PB7
 */
