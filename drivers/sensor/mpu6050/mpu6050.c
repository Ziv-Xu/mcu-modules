/**
 * @file    mpu6050.c
 * @brief   MPU6050 全功能驱动实现
 * @note    所有硬件依赖均抽象为函数指针，遵循 MISRA C 2012 强制规则。
 */

#include "mpu6050.h"

/*---------------------------- 内部常量 ----------------------------------*/
/* 加速度计量程对应的 LSB/g 值 */
static const float AccelSensitivity[4] = {
    16384.0f, /* ±2g  */
    8192.0f,  /* ±4g  */
    4096.0f,  /* ±8g  */
    2048.0f   /* ±16g */
};

/* 陀螺仪量程对应的 LSB/(°/s) 值 */
static const float GyroSensitivity[4] = {
    131.0f, /* ±250°/s  */
    65.5f,  /* ±500°/s  */
    32.8f,  /* ±1000°/s */
    16.4f   /* ±2000°/s */
};

/* 自检出厂调节值对应的掩码 */
#define SELF_TEST_FACTOR_ACCEL (2.0f)  /* 加速度自检分度因子 (供计算用) */
#define SELF_TEST_FACTOR_GYRO  (25.0f) /* 陀螺仪自检分度因子 */

/*---------------------------- 内部辅助函数 ------------------------------*/
/**
 * @brief 向指定寄存器写入单字节
 * @param hmpu  设备句柄
 * @param reg   寄存器地址
 * @param value 数据
 * @return 错误码
 */
static MPU6050_Status WriteReg(MPU6050_HandleTypeDef *hmpu, uint8_t reg, uint8_t value)
{
    if (hmpu == NULL)
    {
        return MPU6050_ERR_INVALID_PARAM;
    }
    if (hmpu->I2C_Write == NULL)
    {
        return MPU6050_ERR_NOT_INIT;
    }
    if (hmpu->I2C_Write(hmpu->DeviceAddr, reg, &value, 1U) != 0)
    {
        return MPU6050_ERR_I2C;
    }
    return MPU6050_OK;
}

/**
 * @brief 从指定寄存器读取单字节
 * @param hmpu  设备句柄
 * @param reg   寄存器地址
 * @param value 输出数据指针
 * @return 错误码
 */
static MPU6050_Status ReadReg(MPU6050_HandleTypeDef *hmpu, uint8_t reg, uint8_t *value)
{
    if ((hmpu == NULL) || (value == NULL))
    {
        return MPU6050_ERR_INVALID_PARAM;
    }
    if (hmpu->I2C_Read == NULL)
    {
        return MPU6050_ERR_NOT_INIT;
    }
    if (hmpu->I2C_Read(hmpu->DeviceAddr, reg, value, 1U) != 0)
    {
        return MPU6050_ERR_I2C;
    }
    return MPU6050_OK;
}

/**
 * @brief 读取多字节寄存器 (连续读)
 * @param hmpu 设备句柄
 * @param reg  起始寄存器地址
 * @param buf  缓冲区
 * @param len  长度
 * @return 错误码
 */
static MPU6050_Status ReadRegs(MPU6050_HandleTypeDef *hmpu, uint8_t reg, uint8_t *buf, uint8_t len)
{
    if ((hmpu == NULL) || (buf == NULL) || (len == 0U))
    {
        return MPU6050_ERR_INVALID_PARAM;
    }
    if (hmpu->I2C_Read == NULL)
    {
        return MPU6050_ERR_NOT_INIT;
    }
    if (hmpu->I2C_Read(hmpu->DeviceAddr, reg, buf, len) != 0)
    {
        return MPU6050_ERR_I2C;
    }
    return MPU6050_OK;
}

/**
 * @brief 修改寄存器中指定位域 (读-修改-写)
 * @param hmpu  设备句柄
 * @param reg   寄存器地址
 * @param mask  掩码 (1表示修改的位)
 * @param value 对应位域值 (已对齐到掩码位置)
 * @return 错误码
 */
static MPU6050_Status ModifyReg(MPU6050_HandleTypeDef *hmpu, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t        tmp;
    MPU6050_Status status = ReadReg(hmpu, reg, &tmp);
    if (status != MPU6050_OK)
    {
        return status;
    }
    tmp = (tmp & (~mask)) | (value & mask);
    return WriteReg(hmpu, reg, tmp);
}

/*----------------------------- 公共 API ----------------------------------*/

/* 注册函数实现（简单赋值） */
MPU6050_Status MPU6050_RegisterI2CReadFunc(MPU6050_HandleTypeDef *hmpu, MPU6050_I2C_ReadFunc read_func)
{
    if (hmpu == NULL)
    {
        return MPU6050_ERR_INVALID_PARAM;
    }
    hmpu->I2C_Read = read_func;
    return MPU6050_OK;
}

MPU6050_Status MPU6050_RegisterI2CWriteFunc(MPU6050_HandleTypeDef *hmpu, MPU6050_I2C_WriteFunc write_func)
{
    if (hmpu == NULL)
    {
        return MPU6050_ERR_INVALID_PARAM;
    }
    hmpu->I2C_Write = write_func;
    return MPU6050_OK;
}

MPU6050_Status MPU6050_RegisterDelayFunc(MPU6050_HandleTypeDef *hmpu, MPU6050_DelayMsFunc delay_func)
{
    if (hmpu == NULL)
    {
        return MPU6050_ERR_INVALID_PARAM;
    }
    hmpu->DelayMs = delay_func;
    return MPU6050_OK;
}

/* 初始化主函数 */
MPU6050_Status MPU6050_Init(MPU6050_HandleTypeDef *hmpu, const MPU6050_ConfigTypeDef *config)
{
    uint8_t        whoami;
    MPU6050_Status status;

    /* 参数检查 */
    if (hmpu == NULL)
    {
        return MPU6050_ERR_INVALID_PARAM;
    }
    if ((hmpu->I2C_Read == NULL) || (hmpu->I2C_Write == NULL))
    {
        return MPU6050_ERR_NOT_INIT;
    }

    /* 默认配置 */
    MPU6050_ConfigTypeDef def_config = {.AccelFullScale = MPU6050_ACCEL_FS_2G,
                                        .GyroFullScale  = MPU6050_GYRO_FS_250,
                                        .DLPFConfig     = MPU6050_DLPF_42HZ,
                                        .ClockSource    = MPU6050_CLK_PLL_X,
                                        .SampleRateDiv  = 0,
#if MPU6050_USE_INTERRUPTS
                                        .IntEnable      = 0x00,
                                        .IntPinLevel    = 0, /* 低有效 */
                                        .IntLatch       = 0, /* 50us脉冲 */
                                        .IntClearOnRead = 0
#endif
    };

    if (config == NULL)
    {
        config = &def_config;
    }

    /* 验证量程范围 */
    if ((config->AccelFullScale > MPU6050_ACCEL_FS_16G) || (config->GyroFullScale > MPU6050_GYRO_FS_2000))
    {
        return MPU6050_ERR_INVALID_PARAM;
    }

    /* 1. 复位设备 (确保从已知状态启动) */
    /*
    先复位的原因和好处：
    1.清除未知或残留状态芯片上电后虽然大部分寄存器有默认值，但若之前曾与其他程序共用I2C总线、或发生过通信错误、或设备处于非正常模式，可能导致某些寄存器处于不可预期状态。软件复位能将所有寄存器恢复为出厂默认值，确保后续配置从确定的基准开始。
    2.终止可能正在进行的内部操作复位会立即中止 FIFO、DMP（如果之前被启用）、传感器数据采集等内部过程，避免新旧配置冲突。
    3.保证时钟与电源状态稳定复位后PWR_MGMT_1的默认值为0x40（仅睡眠位置1），芯片处于低功耗待唤醒状态。随后驱动程序显式配置时钟源（ClockSource）并唤醒设备，可避免因之前错误的时钟设置导致I2C通信异常或传感器数据无效。
    4.提高多实例或热重启的可靠性若系统支持多个MPU6050或需要在不掉电的情况下重新初始化设备，软件复位可以确保每个实例都从相同的初始状态开始，避免因上次运行残留配置造成的意外行为。
    */
    status = WriteReg(hmpu, MPU6050_REG_PWR_MGMT_1, 0x80U); /* 复位所有寄存器 */
    if (status != MPU6050_OK)
        return status;
    if (hmpu->DelayMs != NULL)
    {
        hmpu->DelayMs(100U); /* 等待复位完成 */
    }

    /* 2. 唤醒设备并设置时钟源 */
    status = WriteReg(hmpu, MPU6050_REG_PWR_MGMT_1, (uint8_t) (config->ClockSource & 0x07U));
    if (status != MPU6050_OK)
        return status;
    if (hmpu->DelayMs != NULL)
    {
        hmpu->DelayMs(10U);
    }

    /* 3. 验证 WHO_AM_I */
    status = ReadReg(hmpu, MPU6050_REG_WHO_AM_I, &whoami);
    if (status != MPU6050_OK)
        return status;
    if (whoami != MPU6050_WHO_AM_I_VALUE)
    {
        return MPU6050_ERR_NOT_INIT; /* 设备ID不匹配 */
    }

    /* 4. 设置采样率分频器 */
    status = WriteReg(hmpu, MPU6050_REG_SMPLRT_DIV, config->SampleRateDiv);
    if (status != MPU6050_OK)
        return status;

    /* 5. 设置 DLPF */
    status = WriteReg(hmpu, MPU6050_REG_CONFIG, (uint8_t) (((uint8_t) config->DLPFConfig) & 0x07U));
    if (status != MPU6050_OK)
        return status;

    /* 6. 设置陀螺仪量程 */
    status = WriteReg(hmpu, MPU6050_REG_GYRO_CONFIG, (uint8_t) ((((uint8_t) config->GyroFullScale) << 3) & 0x18U));
    if (status != MPU6050_OK)
        return status;

    /* 7. 设置加速度计量程 */
    status = WriteReg(hmpu, MPU6050_REG_ACCEL_CONFIG, (uint8_t) ((((uint8_t) config->AccelFullScale) << 3) & 0x18U));
    if (status != MPU6050_OK)
        return status;

    /* 保存灵敏度 */
    hmpu->AccelLSB = AccelSensitivity[config->AccelFullScale];
    hmpu->GyroLSB  = GyroSensitivity[config->GyroFullScale];

    /* 8. 配置中断 (若启用) */
#if MPU6050_USE_INTERRUPTS
    if (config->IntEnable != 0U)
    {
        /* 配置中断引脚行为 */
        uint8_t int_cfg = 0U;
        if (config->IntPinLevel != 0U)
        {
            int_cfg |= 0x80U; /* 高有效 */
        }
        if (config->IntLatch != 0U)
        {
            int_cfg |= 0x20U; /* 锁存模式 */
        }
        if (config->IntClearOnRead != 0U)
        {
            int_cfg |= 0x10U; /* 任何读取清除 */
        }
        status = WriteReg(hmpu, MPU6050_REG_INT_PIN_CFG, int_cfg);
        if (status != MPU6050_OK)
            return status;

        /* 使能中断源 */
        status = WriteReg(hmpu, MPU6050_REG_INT_ENABLE, config->IntEnable);
        if (status != MPU6050_OK)
            return status;
    }
#endif

    /* 清除校准偏置 */
#if MPU6050_USE_CALIBRATION
    hmpu->AccelBias[0] = 0;
    hmpu->AccelBias[1] = 0;
    hmpu->AccelBias[2] = 0;
    hmpu->GyroBias[0]  = 0;
    hmpu->GyroBias[1]  = 0;
    hmpu->GyroBias[2]  = 0;
#endif

    hmpu->IsInit = 1U;
    return MPU6050_OK;
}

/* 复位 */
MPU6050_Status MPU6050_Reset(MPU6050_HandleTypeDef *hmpu)
{
    MPU6050_Status status;
    if (hmpu == NULL)
        return MPU6050_ERR_INVALID_PARAM;

    status = WriteReg(hmpu, MPU6050_REG_PWR_MGMT_1, 0x80U);
    if (status != MPU6050_OK)
        return status;
    if (hmpu->DelayMs != NULL)
    {
        hmpu->DelayMs(100U);
    }
    hmpu->IsInit = 0U; /* 需要重新初始化 */
    return MPU6050_OK;
}

/* 唤醒 */
MPU6050_Status MPU6050_WakeUp(MPU6050_HandleTypeDef *hmpu)
{
    if (hmpu == NULL)
        return MPU6050_ERR_INVALID_PARAM;
    return ModifyReg(hmpu, MPU6050_REG_PWR_MGMT_1, 0x40U, 0x00U);
}

/* 读取原始加速度 */
MPU6050_Status MPU6050_GetAccelRaw(MPU6050_HandleTypeDef *hmpu, int16_t accel_adc[3])
{
    uint8_t        buf[6];
    MPU6050_Status status;

    if ((hmpu == NULL) || (accel_adc == NULL))
        return MPU6050_ERR_INVALID_PARAM;
    if (hmpu->IsInit == 0U)
        return MPU6050_ERR_NOT_INIT;

    status = ReadRegs(hmpu, MPU6050_REG_ACCEL_XOUT_H, buf, 6U);
    if (status != MPU6050_OK)
        return status;

    accel_adc[0] = (int16_t) ((uint16_t) buf[0] << 8 | buf[1]);
    accel_adc[1] = (int16_t) ((uint16_t) buf[2] << 8 | buf[3]);
    accel_adc[2] = (int16_t) ((uint16_t) buf[4] << 8 | buf[5]);

    return MPU6050_OK;
}

/* 读取加速度物理值 */
MPU6050_Status MPU6050_GetAccelData(MPU6050_HandleTypeDef *hmpu, float accel_g[3])
{
    int16_t        raw[3];
    MPU6050_Status status = MPU6050_GetAccelRaw(hmpu, raw);
    if (status != MPU6050_OK)
        return status;

    for (uint8_t i = 0U; i < 3U; i++)
    {
        int16_t corrected = raw[i];
#if MPU6050_USE_CALIBRATION
        corrected -= hmpu->AccelBias[i];
#endif
        accel_g[i] = (float) corrected / hmpu->AccelLSB;
    }
    return MPU6050_OK;
}

/* 读取原始陀螺仪 */
MPU6050_Status MPU6050_GetGyroRaw(MPU6050_HandleTypeDef *hmpu, int16_t gyro_adc[3])
{
    uint8_t        buf[6];
    MPU6050_Status status;

    if ((hmpu == NULL) || (gyro_adc == NULL))
        return MPU6050_ERR_INVALID_PARAM;
    if (hmpu->IsInit == 0U)
        return MPU6050_ERR_NOT_INIT;

    status = ReadRegs(hmpu, MPU6050_REG_GYRO_XOUT_H, buf, 6U);
    if (status != MPU6050_OK)
        return status;

    gyro_adc[0] = (int16_t) ((uint16_t) buf[0] << 8 | buf[1]);
    gyro_adc[1] = (int16_t) ((uint16_t) buf[2] << 8 | buf[3]);
    gyro_adc[2] = (int16_t) ((uint16_t) buf[4] << 8 | buf[5]);

    return MPU6050_OK;
}

/* 读取陀螺仪物理值 */
MPU6050_Status MPU6050_GetGyroData(MPU6050_HandleTypeDef *hmpu, float gyro_dps[3])
{
    int16_t        raw[3];
    MPU6050_Status status = MPU6050_GetGyroRaw(hmpu, raw);
    if (status != MPU6050_OK)
        return status;

    for (uint8_t i = 0U; i < 3U; i++)
    {
        int16_t corrected = raw[i];
#if MPU6050_USE_CALIBRATION
        corrected -= hmpu->GyroBias[i];
#endif
        gyro_dps[i] = (float) corrected / hmpu->GyroLSB;
    }
    return MPU6050_OK;
}

/* 读取温度 */
MPU6050_Status MPU6050_GetTemperature(MPU6050_HandleTypeDef *hmpu, float *temp_c)
{
    uint8_t        buf[2];
    MPU6050_Status status;

    if ((hmpu == NULL) || (temp_c == NULL))
        return MPU6050_ERR_INVALID_PARAM;
    if (hmpu->IsInit == 0U)
        return MPU6050_ERR_NOT_INIT;

    status = ReadRegs(hmpu, MPU6050_REG_TEMP_OUT_H, buf, 2U);
    if (status != MPU6050_OK)
        return status;

    int16_t raw = (int16_t) ((uint16_t) buf[0] << 8 | buf[1]);
    *temp_c     = ((float) raw / 340.0f) + 36.53f;
    return MPU6050_OK;
}

/* ------------ 自检实现 ------------ */
#if MPU6050_USE_SELF_TEST
MPU6050_Status MPU6050_SelfTest(MPU6050_HandleTypeDef *hmpu, float accel_bias_pct[3], float gyro_bias_pct[3])
{
    uint8_t       self_test_data[6]; /* 自检寄存器 X/Y/Z 和 A */
    int16_t       self_test_tap[6];  /* 存储未处理的出厂值 */
    int16_t       response[6];       /* 自检响应 = 自检启用时读数 - 禁用时读数 */
    int32_t       sum_no_st[6] = {0};
    int32_t       sum_st[6]    = {0};
    const uint8_t num_samples  = 50U;

    if ((hmpu == NULL) || (accel_bias_pct == NULL) || (gyro_bias_pct == NULL))
    {
        return MPU6050_ERR_INVALID_PARAM;
    }
    if (hmpu->IsInit == 0U)
        return MPU6050_ERR_NOT_INIT;

    /* 备份当前配置 (简要) */
    uint8_t prev_config, prev_gyro, prev_accel;
    ReadReg(hmpu, MPU6050_REG_CONFIG, &prev_config);
    ReadReg(hmpu, MPU6050_REG_GYRO_CONFIG, &prev_gyro);
    ReadReg(hmpu, MPU6050_REG_ACCEL_CONFIG, &prev_accel);

    /* 自检要求：加速度计设置 ±8g，陀螺仪设置 ±250dps，DLPF=188Hz */
    WriteReg(hmpu, MPU6050_REG_ACCEL_CONFIG, 0x10U); /* ±8g */
    WriteReg(hmpu, MPU6050_REG_GYRO_CONFIG, 0x00U);  /* ±250dps */
    WriteReg(hmpu, MPU6050_REG_CONFIG, 0x01U);       /* DLPF_188Hz */
    if (hmpu->DelayMs != NULL)
        hmpu->DelayMs(20U);

    /* 读取禁用自检时的平均值 */
    WriteReg(hmpu, MPU6050_REG_ACCEL_CONFIG, 0x10U); /* 确保自检位为0 */
    WriteReg(hmpu, MPU6050_REG_GYRO_CONFIG, 0x00U);
    for (uint8_t n = 0U; n < num_samples; n++)
    {
        int16_t raw[6];
        MPU6050_GetAccelRaw(hmpu, &raw[0]);
        MPU6050_GetGyroRaw(hmpu, &raw[3]);
        for (uint8_t i = 0U; i < 6U; i++) sum_no_st[i] += raw[i];
        if (hmpu->DelayMs != NULL)
            hmpu->DelayMs(2U);
    }

    /* 启用自检位 (加速度和陀螺仪自检同时) */
    WriteReg(hmpu, MPU6050_REG_ACCEL_CONFIG, 0xE0U | 0x10U);
    WriteReg(hmpu, MPU6050_REG_GYRO_CONFIG, 0xE0U | 0x00U);
    if (hmpu->DelayMs != NULL)
        hmpu->DelayMs(20U);

    for (uint8_t n = 0U; n < num_samples; n++)
    {
        int16_t raw[6];
        MPU6050_GetAccelRaw(hmpu, &raw[0]);
        MPU6050_GetGyroRaw(hmpu, &raw[3]);
        for (uint8_t i = 0U; i < 6U; i++) sum_st[i] += raw[i];
        if (hmpu->DelayMs != NULL)
            hmpu->DelayMs(2U);
    }

    /* 计算自检响应 */
    for (uint8_t i = 0U; i < 6U; i++)
    {
        int16_t avg_no = (int16_t) (sum_no_st[i] / num_samples);
        int16_t avg_st = (int16_t) (sum_st[i] / num_samples);
        response[i]    = avg_st - avg_no;
    }

    /* 恢复配置 */
    WriteReg(hmpu, MPU6050_REG_CONFIG, prev_config);
    WriteReg(hmpu, MPU6050_REG_GYRO_CONFIG, prev_gyro);
    WriteReg(hmpu, MPU6050_REG_ACCEL_CONFIG, prev_accel);

    /* 读取自检出厂调节值 */
    ReadRegs(hmpu, MPU6050_REG_SELF_TEST_X, self_test_data, 6U);
    self_test_tap[0] = (int16_t) (((uint16_t) self_test_data[0] & 0xE0U) << 8); /* 陀螺仪 */
    self_test_tap[1] = (int16_t) (((uint16_t) self_test_data[1] & 0xE0U) << 8);
    self_test_tap[2] = (int16_t) (((uint16_t) self_test_data[2] & 0xE0U) << 8);
    self_test_tap[3] = (int16_t) (((uint16_t) self_test_data[3] & 0xE0U) << 8)
                       | (int16_t) ((uint16_t) self_test_data[0] & 0x1FU); /* 加速度 */
    self_test_tap[4] =
        (int16_t) (((uint16_t) self_test_data[4] & 0xE0U) << 8) | (int16_t) ((uint16_t) self_test_data[1] & 0x1FU);
    self_test_tap[5] =
        (int16_t) (((uint16_t) self_test_data[5] & 0xE0U) << 8) | (int16_t) ((uint16_t) self_test_data[2] & 0x1FU);

    /* 计算偏差百分比: (响应 - 出厂值) / 出厂值 * 100% */
    for (uint8_t i = 0U; i < 3U; i++)
    {
        if (self_test_tap[i] != 0)
        {
            gyro_bias_pct[i] = 100.0f * (float) (response[i] - self_test_tap[i]) / (float) self_test_tap[i];
        }
        else
        {
            gyro_bias_pct[i] = 0.0f;
        }
    }
    for (uint8_t i = 3U; i < 6U; i++)
    {
        if (self_test_tap[i] != 0)
        {
            accel_bias_pct[i - 3] = 100.0f * (float) (response[i] - self_test_tap[i]) / (float) self_test_tap[i];
        }
        else
        {
            accel_bias_pct[i - 3] = 0.0f;
        }
    }
    return MPU6050_OK;
}
#endif /* MPU6050_USE_SELF_TEST */

/* ------------ 零偏校准 ------------ */
#if MPU6050_USE_CALIBRATION
MPU6050_Status MPU6050_Calibrate(MPU6050_HandleTypeDef *hmpu, uint16_t samples)
{
    int32_t sum_accel[3] = {0};
    int32_t sum_gyro[3]  = {0};
    int16_t raw_accel[3], raw_gyro[3];

    if ((hmpu == NULL) || (samples == 0U))
        return MPU6050_ERR_INVALID_PARAM;
    if (hmpu->IsInit == 0U)
        return MPU6050_ERR_NOT_INIT;

    for (uint16_t n = 0U; n < samples; n++)
    {
        MPU6050_Status status;
        status = MPU6050_GetAccelRaw(hmpu, raw_accel);
        if (status != MPU6050_OK)
            return status;
        status = MPU6050_GetGyroRaw(hmpu, raw_gyro);
        if (status != MPU6050_OK)
            return status;

        for (uint8_t i = 0U; i < 3U; i++)
        {
            sum_accel[i] += raw_accel[i];
            sum_gyro[i] += raw_gyro[i];
        }
        if (hmpu->DelayMs != NULL)
        {
            hmpu->DelayMs(2U);
        }
    }

    /* 计算平均值并存储偏置 */
    for (uint8_t i = 0U; i < 3U; i++)
    {
        hmpu->AccelBias[i] = (int16_t) (sum_accel[i] / (int32_t) samples);
        hmpu->GyroBias[i]  = (int16_t) (sum_gyro[i] / (int32_t) samples);
    }
    /* 加速度计校准通常需要保留 Z 轴扣除重力，用户可自行调整。
       此处仅提供基础平均偏置，Z 轴需后期处理。 */
    return MPU6050_OK;
}

void MPU6050_ResetCalibration(MPU6050_HandleTypeDef *hmpu)
{
    if (hmpu != NULL)
    {
        hmpu->AccelBias[0] = 0;
        hmpu->AccelBias[1] = 0;
        hmpu->AccelBias[2] = 0;
        hmpu->GyroBias[0]  = 0;
        hmpu->GyroBias[1]  = 0;
        hmpu->GyroBias[2]  = 0;
    }
}
#endif /* MPU6050_USE_CALIBRATION */

/* ------------ 中断管理 ------------ */
#if MPU6050_USE_INTERRUPTS
MPU6050_Status MPU6050_SetInterrupts(MPU6050_HandleTypeDef *hmpu, uint8_t int_mask)
{
    if (hmpu == NULL)
        return MPU6050_ERR_INVALID_PARAM;
    return WriteReg(hmpu, MPU6050_REG_INT_ENABLE, int_mask & 0x1DU);
}

MPU6050_Status MPU6050_GetIntStatus(MPU6050_HandleTypeDef *hmpu, uint8_t *status)
{
    if ((hmpu == NULL) || (status == NULL))
        return MPU6050_ERR_INVALID_PARAM;
    return ReadReg(hmpu, MPU6050_REG_INT_STATUS, status);
}

MPU6050_Status MPU6050_SetMotionDetection(MPU6050_HandleTypeDef *hmpu, uint8_t threshold, uint8_t duration)
{
    MPU6050_Status status;
    status = WriteReg(hmpu, MPU6050_REG_MOT_THR, threshold);
    if (status != MPU6050_OK)
        return status;
    return WriteReg(hmpu, MPU6050_REG_MOT_DUR, duration);
}

MPU6050_Status MPU6050_SetZeroMotionDetection(MPU6050_HandleTypeDef *hmpu, uint8_t threshold, uint8_t duration)
{
    MPU6050_Status status;
    status = WriteReg(hmpu, MPU6050_REG_ZRMOT_THR, threshold);
    if (status != MPU6050_OK)
        return status;
    return WriteReg(hmpu, MPU6050_REG_ZRMOT_DUR, duration);
}

MPU6050_Status MPU6050_SetFreeFallDetection(MPU6050_HandleTypeDef *hmpu, uint8_t threshold, uint8_t duration)
{
    MPU6050_Status status;
    status = WriteReg(hmpu, MPU6050_REG_FF_THR, threshold);
    if (status != MPU6050_OK)
        return status;
    return WriteReg(hmpu, MPU6050_REG_FF_DUR, duration);
}
#endif /* MPU6050_USE_INTERRUPTS */

/* ------------ 低功耗 ------------ */
#if MPU6050_USE_LOW_POWER
MPU6050_Status MPU6050_EnterSleep(MPU6050_HandleTypeDef *hmpu)
{
    if (hmpu == NULL)
        return MPU6050_ERR_INVALID_PARAM;
    return ModifyReg(hmpu, MPU6050_REG_PWR_MGMT_1, 0x40U, 0x40U);
}

MPU6050_Status MPU6050_SetCycleMode(MPU6050_HandleTypeDef *hmpu, uint8_t awake_freq)
{
    if (hmpu == NULL)
        return MPU6050_ERR_INVALID_PARAM;
    if (awake_freq > 4U)
        return MPU6050_ERR_INVALID_PARAM;

    /* 使能 Cycle 模式并设置频率，同时关闭陀螺仪 */
    uint8_t        val    = 0x20U | (awake_freq << 6); /* CYCLE=1, LP_WAKE_CTRL=频率 */
    MPU6050_Status status = WriteReg(hmpu, MPU6050_REG_PWR_MGMT_1, val);
    if (status != MPU6050_OK)
        return status;
    /* 关闭陀螺仪各轴 (PWR_MGMT_2) */
    return WriteReg(hmpu, MPU6050_REG_PWR_MGMT_2, 0x07U);
}
#endif /* MPU6050_USE_LOW_POWER */

/* ------------ FIFO ------------ */
#if MPU6050_USE_FIFO
MPU6050_Status MPU6050_SetFIFOConfig(MPU6050_HandleTypeDef *hmpu, uint8_t accel_en, uint8_t gyro_en, uint8_t temp_en)
{
    uint8_t reg = 0U;
    if (accel_en != 0U)
        reg |= 0x08U;
    if (gyro_en != 0U)
        reg |= 0x70U; /* X/Y/Z 三轴 */
    if (temp_en != 0U)
        reg |= 0x80U;
    return WriteReg(hmpu, MPU6050_REG_FIFO_EN, reg);
}

MPU6050_Status MPU6050_GetFIFOCount(MPU6050_HandleTypeDef *hmpu, uint16_t *count)
{
    uint8_t        buf[2];
    MPU6050_Status status;
    if ((hmpu == NULL) || (count == NULL))
        return MPU6050_ERR_INVALID_PARAM;

    status = ReadRegs(hmpu, MPU6050_REG_FIFO_COUNTH, buf, 2U);
    if (status != MPU6050_OK)
        return status;
    *count = ((uint16_t) buf[0] << 8) | buf[1];
    return MPU6050_OK;
}

MPU6050_Status MPU6050_ReadFIFO(MPU6050_HandleTypeDef *hmpu, uint8_t *buf, uint16_t len)
{
    uint8_t int_status;
    if ((hmpu == NULL) || (buf == NULL) || (len == 0U))
        return MPU6050_ERR_INVALID_PARAM;

    /* 检查 FIFO 溢出 */
    ReadReg(hmpu, MPU6050_REG_INT_STATUS, &int_status);
    if ((int_status & 0x10U) != 0U)
    {
        /* 溢出发生，复位 FIFO */
        MPU6050_ResetFIFO(hmpu);
        return MPU6050_ERR_FIFO_OVF;
    }

    /* 多字节读取 FIFO 数据寄存器 */
    return hmpu->I2C_Read(hmpu->DeviceAddr, MPU6050_REG_FIFO_R_W, buf, (uint8_t) len) == 0 ? MPU6050_OK
                                                                                           : MPU6050_ERR_I2C;
}

MPU6050_Status MPU6050_ResetFIFO(MPU6050_HandleTypeDef *hmpu)
{
    if (hmpu == NULL)
        return MPU6050_ERR_INVALID_PARAM;
    /* 设置 USER_CTRL 的 FIFO 复位位 */
    return ModifyReg(hmpu, MPU6050_REG_USER_CTRL, 0x04U, 0x04U);
}
#endif /* MPU6050_USE_FIFO */

/* ------------ DMP (框架) ------------ */
#if MPU6050_USE_DMP
/**
 * @brief 加载 DMP 固件 (用户需提供官方 InvenSense 固件数组)
 * @note 用户可将固件数组定义为 'dmp_firmware' 并实现此函数。
 *       典型流程：向 MPU6050 的 DMP 存储器写入固件块。
 *       由于版权原因，本驱动不包含固件数组。
 */
static MPU6050_Status MPU6050_DMP_LoadFirmware(MPU6050_HandleTypeDef *hmpu)
{
    /*
     * 示例伪代码:
     * extern const uint8_t dmp_firmware[];
     * extern const uint16_t dmp_firmware_len;
     * for (uint16_t i = 0; i < dmp_firmware_len; i++) {
     *     WriteReg(hmpu, MPU6050_DMP_MEM_START + i, dmp_firmware[i]);
     * }
     * 用户可参考官方 "motion_driver" 中的加载流程。
     */
    (void) hmpu;
    return MPU6050_ERR_DMP_LOAD; /* 需要用户实现 */
}

MPU6050_Status MPU6050_DMP_Init(MPU6050_HandleTypeDef *hmpu)
{
    if (hmpu == NULL)
        return MPU6050_ERR_INVALID_PARAM;
    MPU6050_Status status;

    /* 1. 唤醒并设置时钟 */
    status = MPU6050_WakeUp(hmpu);
    if (status != MPU6050_OK)
        return status;

    /* 2. 加载 DMP 固件 */
    status = MPU6050_DMP_LoadFirmware(hmpu);
    if (status != MPU6050_OK)
        return status;

    /* 3. 设置 DMP 配置寄存器 (采样率、FIFO 输出等) */
    /* 此处应设置 DMP 对应的内存寄存器，省略细节 */

    /* 4. 使能 DMP 并启动 */
    uint8_t user_ctrl = 0xC0U; /* DMP_EN + FIFO_EN */
    status            = WriteReg(hmpu, MPU6050_REG_USER_CTRL, user_ctrl);
    if (status != MPU6050_OK)
        return status;

    hmpu->DmpEnabled = 1U;
    return MPU6050_OK;
}

MPU6050_Status MPU6050_DMP_GetQuaternion(MPU6050_HandleTypeDef *hmpu, float q[4])
{
    if (hmpu == NULL || q == NULL)
        return MPU6050_ERR_INVALID_PARAM;
    /* DMP 四元数通常存储在 FIFO 中，解析后返回。
       此处返回未实现错误。 */
    return MPU6050_ERR_NOT_INIT;
}

MPU6050_Status MPU6050_DMP_GetEuler(MPU6050_HandleTypeDef *hmpu, float *yaw, float *pitch, float *roll)
{
    /* 从四元数计算欧拉角 */
    float          q[4];
    MPU6050_Status status = MPU6050_DMP_GetQuaternion(hmpu, q);
    if (status != MPU6050_OK)
        return status;
    /* 简化计算: 假设使用标准 Z-Y-X 顺序 */
    /* 实际实现需添加转换公式 */
    return MPU6050_ERR_NOT_INIT;
}

MPU6050_Status MPU6050_DMP_GetAccel(MPU6050_HandleTypeDef *hmpu, float accel[3])
{
    (void) hmpu;
    (void) accel;
    return MPU6050_ERR_NOT_INIT;
}

MPU6050_Status MPU6050_DMP_GetGyro(MPU6050_HandleTypeDef *hmpu, float gyro[3])
{
    (void) hmpu;
    (void) gyro;
    return MPU6050_ERR_NOT_INIT;
}
#endif /* MPU6050_USE_DMP */

/* ------------ 辅助 I2C 直通模式 ------------ */
#if MPU6050_USE_AUX_I2C
MPU6050_Status MPU6050_SetBypassMode(MPU6050_HandleTypeDef *hmpu, uint8_t enable)
{
    if (hmpu == NULL)
        return MPU6050_ERR_INVALID_PARAM;
    uint8_t mask = 0x20U; /* BYPASS_EN */
    return ModifyReg(hmpu, MPU6050_REG_INT_PIN_CFG, mask, enable ? mask : 0U);
}
#endif /* MPU6050_USE_AUX_I2C */
