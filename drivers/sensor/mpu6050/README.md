
# MPU6050 全功能驱动模块

[![Language](https://img.shields.io/badge/Language-C99-blue.svg)](#)
[![Standard](https://img.shields.io/badge/Standard-MISRA_C_2012-green.svg)](#)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](#)

---
## 基本知识提要

- 其中mpu6050的fifo是内部集成了一个 1024 字节的硬件 FIFO 缓冲区，专门用于缓存加速度、陀螺仪和温度数据。它完全由芯片自身管理，不需要再引入外部软件FIFO代码（如本仓库中的utils）

- DMP模式（不经过 MCU）：由MPU6050内部的DMP直接读取磁力计数据，自动完成9轴融合，输出四元数。MCU 无需关心磁力计。

- 辅助 I2C 直通模式（Bypass Mode）是 MPU6050 提供的一种硬件桥接功能。  
简单来说，它把mpu6050上的一组辅助I2C总线（AUX_CL / AUX_DA也就是丝印上的xcl和xda）直接短接到主I2C总线上，让主控MCU能够像访问同一总线的设备一样，直接与挂在辅助总线上的传感器（如磁力计）通信。相当于i2c的一主多从  
一般用来连接外置的三轴磁力计，使6050的六轴拓展成九轴。  
此模式和DMP模式互斥，不能混合使用。

---

## 主要特性

- **硬件完全抽象**  
  所有 I2C 操作和延时均通过**函数指针**实现，无任何平台 HAL 库依赖。  
  用户只需实现：`I2C读`、`I2C写`、`毫秒延时`。

- **多实例安全**  
  使用句柄结构体 (`MPU6050_HandleTypeDef`) 管理全部状态，可同时驱动多个 MPU6050，无全局变量冲突。

- **全功能覆盖 & 按需裁剪**  
  通过配置宏 (`MPU6050_USE_XXX`) 启用或关闭功能模块：
  - 基本传感器读取（加速度 / 陀螺仪 / 温度）
  - 自检 (Self-Test)
  - 零偏自动校准
  - 可编程中断（数据就绪、运动、静止、自由落体）
  - 低功耗模式（睡眠、循环模式、加速度计低功耗）
  - FIFO 操作
  - DMP 数字运动处理器（四元数、欧拉角）
  - 辅助 I2C 直通模式（Bypass）

- **高代码质量**  
  - 遵循 **MISRA C:2012** 强制规则  
  - 全部整数类型使用 `stdint.h`，无隐式提升风险  
  - 完整的 **Doxygen** 注释与**统一错误码**  
  - 所有寄存器地址使用预定义宏，无魔数

- **零依赖**  
  除标准库头文件 `<stdint.h>` 外无任何外部依赖。可直接集成到任何 C 工程。

---

## 快速开始

### 1. 实现硬件接口函数
根据你的平台实现三个函数：

```c
int32_t my_i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len) 
{
    // 使用你的 I2C 库进行连续读操作，返回 0 表示成功
}

int32_t my_i2c_write(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint8_t len) 
{
    // 使用你的 I2C 库进行连续写操作，返回 0 表示成功
}

void my_delay_ms(uint32_t ms) 
{
    // 毫秒级延时
}
```

### 2. 初始化并使用 MPU6050

```c
#include "mpu6050.h"

int main(void) 
{
    MPU6050_HandleTypeDef mpu;
    MPU6050_ConfigTypeDef config = {0};
    float accel[3], gyro[3];

    // 填充句柄
    mpu.DeviceAddr = MPU6050_DEFAULT_DEVICE_ADDR;
    //可以通过注册函数赋值
    /*
    MPU6050_RegisterI2CReadFunc(&mpu, my_i2c_read);
    MPU6050_RegisterI2CWriteFunc(&mpu, my_i2c_write);
    MPU6050_RegisterDelayFunc(&mpu, my_delay_ms);
    */
    //也可以直接赋值
    mpu.I2C_Read   = my_i2c_read;
    mpu.I2C_Write  = my_i2c_write;
    mpu.DelayMs    = my_delay_ms;

    // 配置参数（也可传 NULL 使用默认值）
    config.AccelFullScale = MPU6050_ACCEL_FS_4G;
    config.GyroFullScale  = MPU6050_GYRO_FS_1000;
    config.DLPFConfig     = MPU6050_DLPF_42HZ;
    config.ClockSource    = MPU6050_CLK_PLL_X;

    if (MPU6050_Init(&mpu, &config) != MPU6050_OK) {
        // 错误处理
    }

    // 可选：执行零偏校准（保持静止）
    #if MPU6050_USE_CALIBRATION
    MPU6050_Calibrate(&mpu, 200);
    #endif

    while (1) {
        MPU6050_GetAccelData(&mpu, accel);
        MPU6050_GetGyroData(&mpu, gyro);
        // 使用 accel[0], accel[1], accel[2] (单位 g)
        // gyro[0], gyro[1], gyro[2] (单位 °/s)
        my_delay_ms(10);
    }
}
```

---

## 移植指南

### 需要用户实现的接口

| 函数类型 | 函数原型 | 描述 |
|----------|----------|------|
| **I2C 读** | `int32_t func(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len)` | 从 `dev_addr` 的 `reg_addr` 寄存器起读取 `len` 字节到 `data`。成功返回 0。 |
| **I2C 写** | `int32_t func(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint8_t len)` | 将 `data` 中的 `len` 字节写入 `dev_addr` 的 `reg_addr`。成功返回 0。 |
| **毫秒延时** | `void func(uint32_t ms)` | 提供至少毫秒级精度的阻塞延时。 |

> **注意**：`dev_addr` 已经是 7 位地址左移 1 位后的值（如 `0xD0`），可直接用于标准 I2C 库。

### 集成步骤
1. 将 `mpu6050.h` 和 `mpu6050.c` 加入工程。
2. 实现上述三个函数（通常利用你已有的软件 I2C 或硬件 I2C 封装）。
3. 在调用 `MPU6050_Init` 前，通过句柄成员或 `MPU6050_Register...` 函数设置指针。
4. 根据需求在编译选项中定义功能裁剪宏（或直接修改 `.h` 文件中的默认值）。

---

## 功能裁剪宏

所有高级功能均可通过宏独立启用/禁用，避免代码膨胀。  
**默认值**见下表，可在 `mpu6050.h` 开头或编译参数中覆盖。

| 宏定义 | 默认值 | 描述 |
|--------|--------|------|
| `MPU6050_USE_SELF_TEST` | 1 | 自检功能 |
| `MPU6050_USE_CALIBRATION` | 1 | 零偏自动校准 |
| `MPU6050_USE_INTERRUPTS` | 1 | 运动/静止/自由落体中断 |
| `MPU6050_USE_FIFO` | 0 | FIFO 缓冲操作 |
| `MPU6050_USE_DMP` | 0 | 数字运动处理器 |
| `MPU6050_USE_LOW_POWER` | 1 | 睡眠/循环模式 |
| `MPU6050_USE_AUX_I2C` | 1 | 辅助 I2C 直通（读磁力计） |

---

## API 概览

### 初始化和配置
| 函数 | 描述 |
|------|------|
| `MPU6050_Init` | 器件复位、WHO_AM_I 验证、应用配置 |
| `MPU6050_Reset` | 硬复位所有寄存器 |
| `MPU6050_WakeUp` | 唤醒设备 |

### 数据获取
| 函数 | 描述 |
|------|------|
| `MPU6050_GetAccelRaw` | 原始 ADC 值 |
| `MPU6050_GetAccelData` | 物理值（g，自动扣除校准偏置） |
| `MPU6050_GetGyroRaw` | 原始 ADC 值 |
| `MPU6050_GetGyroData` | 物理值（°/s，自动扣除校准偏置） |
| `MPU6050_GetTemperature` | 温度值（℃） |

### 自检与校准（需开启对应宏）
| 函数 | 描述 |
|------|------|
| `MPU6050_SelfTest` | 执行自检，返回与出厂值的偏差百分比 |
| `MPU6050_Calibrate` | 静态零偏校准（多次采样求均） |
| `MPU6050_ResetCalibration` | 清空校准数据 |

### 中断管理（需开启 `MPU6050_USE_INTERRUPTS`）
| 函数 | 描述 |
|------|------|
| `MPU6050_SetInterrupts` | 配置中断源使能 |
| `MPU6050_GetIntStatus` | 读取当前中断状态 |
| `MPU6050_SetMotionDetection` | 运动检测阈值与持续时间 |
| `MPU6050_SetZeroMotionDetection` | 静止检测参数 |
| `MPU6050_SetFreeFallDetection` | 自由落体检测参数 |

### 低功耗（需开启 `MPU6050_USE_LOW_POWER`）
| 函数 | 描述 |
|------|------|
| `MPU6050_EnterSleep` | 进入睡眠模式 |
| `MPU6050_SetCycleMode` | 配置加速度计循环模式及唤醒频率 |

### FIFO（需开启 `MPU6050_USE_FIFO`）
| 函数 | 描述 |
|------|------|
| `MPU6050_SetFIFOConfig` | 选择 FIFO 存储内容 |
| `MPU6050_GetFIFOCount` | 读取 FIFO 中现有字节数 |
| `MPU6050_ReadFIFO` | 读取 FIFO 数据 |
| `MPU6050_ResetFIFO` | 清空 FIFO |

### DMP（需开启 `MPU6050_USE_DMP`）
| 函数 | 描述 |
|------|------|
| `MPU6050_DMP_Init` | 加载并启动 DMP（需用户提供固件） |
| `MPU6050_DMP_GetQuaternion` | 获取四元数 |
| `MPU6050_DMP_GetEuler` | 获取欧拉角 |
| `MPU6050_DMP_GetAccel` | DMP 输出处理后的加速度 |
| `MPU6050_DMP_GetGyro` | DMP 输出处理后的陀螺仪 |

### 辅助 I2C（需开启 `MPU6050_USE_AUX_I2C`）
| 函数 | 描述 |
|------|------|
| `MPU6050_SetBypassMode` | 使能/禁用直通模式 |

---

## 错误码说明

所有 API 返回值均为 `MPU6050_Status` 枚举：

| 错误码 | 值 | 描述 |
|--------|----|------|
| `MPU6050_OK` | 0 | 成功 |
| `MPU6050_ERR_I2C` | -1 | I2C 通信错误 |
| `MPU6050_ERR_INVALID_PARAM` | -2 | 无效参数（空指针等） |
| `MPU6050_ERR_TIMEOUT` | -3 | 操作超时 |
| `MPU6050_ERR_NOT_INIT` | -4 | 设备未初始化或未注册接口函数 |
| `MPU6050_ERR_SELFTEST` | -5 | 自检失败 |
| `MPU6050_ERR_DMP_LOAD` | -6 | DMP 固件加载失败 |
| `MPU6050_ERR_FIFO_OVF` | -7 | FIFO 溢出 |

---

## 目录结构

```
├── mpu6050.h          # 头文件（接口声明、配置宏、结构体定义）
├── mpu6050.c          # 实现文件（全功能驱动源码）
└── README.md          # 本文档
```

---

## 注意事项

1. **DMP 固件**  
   由于 InvenSense 版权限制，本驱动不提供 DMP 固件数组。  
   若启用 `MPU6050_USE_DMP`，用户需自行获取官方固件并实现 `MPU6050_DMP_LoadFirmware` 函数。  
   提示：许多开源项目（如 `i2cdevlib`）提供了兼容的固件数组。

2. **校准参考**  
   `MPU6050_Calibrate` 计算的是各轴的均值偏置。  
   对于加速度计的 Z 轴，建议在水平静止状态下校准，重力分量会被计入偏置，用户可根据需要手动修正。

3. **中断引脚配置**  
   中断引脚的有效电平、锁存方式和清除方式在初始化结构体中设定，需根据外部电路配合使用。

4. **I2C 通信速率**  
   建议使用 400kHz Fast Mode。部分功能（如 DMP）对时序要求较高，过低速率可能导致数据溢出。

---

## 版本

- **v1.0.0** — 初始发布，涵盖所有基础及高级功能。

---

## 许可证

本项目采用 **MIT 许可证**，允许自由使用、修改和商用，只需保留原作者版权声明。
