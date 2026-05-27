/**
 * @file    mpu6050.h
 * @brief   MPU6050 全功能驱动头文件 (高可移植性、多实例)
 * @details 用户可通过配置宏裁剪功能，所有硬件操作均抽象为函数指针。
 *          使用 Doxygen 风格注释。
 * @note    请在使用前实现并注册 I2C 读写及延时函数。
 * @version 1.0.0
 */

#ifndef __MPU6050_H__
#define __MPU6050_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------- 包含 -----------------------------------*/
#include <stdint.h>
#include <stddef.h>

    /*---------------------------- 功能裁剪宏 --------------------------------*/
    /* 用户可通过全局定义或在包含此头文件前定义以下宏来裁剪功能。
       若未定义，则使用默认值（开启校准和中断，关闭 FIFO/DMP/低功耗等） */

#ifndef MPU6050_USE_SELF_TEST
#define MPU6050_USE_SELF_TEST 1 /**< 自检功能 */
#endif

#ifndef MPU6050_USE_CALIBRATION
#define MPU6050_USE_CALIBRATION 1 /**< 零偏校准功能 */
#endif

#ifndef MPU6050_USE_INTERRUPTS
#define MPU6050_USE_INTERRUPTS 1 /**< 中断检测功能 */
#endif

#ifndef MPU6050_USE_FIFO
#define MPU6050_USE_FIFO 0 /**< FIFO 功能 */
#endif

#ifndef MPU6050_USE_DMP
#define MPU6050_USE_DMP 0 /**< DMP 数字运动处理器 */
#endif

#ifndef MPU6050_USE_LOW_POWER
#define MPU6050_USE_LOW_POWER 1 /**< 低功耗模式管理 */
#endif

#ifndef MPU6050_USE_AUX_I2C
#define MPU6050_USE_AUX_I2C 1 /**< 辅助 I2C 接口 */
#endif

/*------------------------------- 公共宏 --------------------------------*/
#define MPU6050_DEFAULT_DEVICE_ADDR 0x68U /**< 默认 I2C 地址 (AD0=0) */
#define MPU6050_ALTERNATE_ADDR      0x69U /**< 备用 I2C 地址 (AD0=1) */
#define MPU6050_WHO_AM_I_VALUE      0x68U /**< WHO_AM_I 寄存器期望值 */

    /*------------------------------- 错误码 ---------------------------------*/
    typedef enum
    {
        MPU6050_OK                = 0,  /**< 操作成功 */
        MPU6050_ERR_I2C           = -1, /**< I2C 通信错误 */
        MPU6050_ERR_INVALID_PARAM = -2, /**< 参数无效 */
        MPU6050_ERR_TIMEOUT       = -3, /**< 操作超时 */
        MPU6050_ERR_NOT_INIT      = -4, /**< 设备未初始化 */
        MPU6050_ERR_SELFTEST      = -5, /**< 自检失败 */
        MPU6050_ERR_DMP_LOAD      = -6, /**< DMP 固件加载失败 */
        MPU6050_ERR_FIFO_OVF      = -7  /**< FIFO 溢出 */
    } MPU6050_Status;

    /*------------------------ I2C 函数指针类型定义 --------------------------*/
    /**
     * @brief I2C 读操作函数原型
     * @param dev_addr  设备 I2C 地址 (7位地址左移1位)
     * @param reg_addr  寄存器地址
     * @param data      读取数据缓冲区
     * @param len       读取长度
     * @return 0 - 成功，其他 - 错误码
     */
    typedef int32_t (*MPU6050_I2C_ReadFunc)(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len);

    /**
     * @brief I2C 写操作函数原型
     * @param dev_addr  设备 I2C 地址
     * @param reg_addr  寄存器地址
     * @param data      写入数据缓冲区
     * @param len       写入长度
     * @return 0 - 成功，其他 - 错误码
     */
    typedef int32_t (*MPU6050_I2C_WriteFunc)(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint8_t len);

    /**
     * @brief 毫秒延时函数原型
     * @param ms 延时毫秒数
     */
    typedef void (*MPU6050_DelayMsFunc)(uint32_t ms);

    /*---------------------------- 枚举类型定义 -------------------------------*/
    /** @brief 加速度计量程选择 */
    typedef enum
    {
        MPU6050_ACCEL_FS_2G  = 0x00, /**< ±2g   */
        MPU6050_ACCEL_FS_4G  = 0x01, /**< ±4g   */
        MPU6050_ACCEL_FS_8G  = 0x02, /**< ±8g   */
        MPU6050_ACCEL_FS_16G = 0x03  /**< ±16g  */
    } MPU6050_AccelFullScale;

    /** @brief 陀螺仪量程选择 */
    typedef enum
    {
        MPU6050_GYRO_FS_250  = 0x00, /**< ±250°/s  */
        MPU6050_GYRO_FS_500  = 0x01, /**< ±500°/s  */
        MPU6050_GYRO_FS_1000 = 0x02, /**< ±1000°/s */
        MPU6050_GYRO_FS_2000 = 0x03  /**< ±2000°/s */
    } MPU6050_GyroFullScale;

    /** @brief DLPF 数字低通滤波器配置 */
    typedef enum
    {
        MPU6050_DLPF_256HZ = 0x00, /**< 带宽 256Hz (加速度) / 256Hz (陀螺仪), 延迟 0.98ms */
        MPU6050_DLPF_188HZ = 0x01, /**< 带宽 188Hz / 188Hz, 延迟 1.9ms */
        MPU6050_DLPF_98HZ  = 0x02, /**< 带宽 98Hz / 98Hz, 延迟 2.8ms */
        MPU6050_DLPF_42HZ  = 0x03, /**< 带宽 42Hz / 42Hz, 延迟 4.8ms */
        MPU6050_DLPF_20HZ  = 0x04, /**< 带宽 20Hz / 20Hz, 延迟 8.3ms */
        MPU6050_DLPF_10HZ  = 0x05, /**< 带宽 10Hz / 10Hz, 延迟 13.4ms */
        MPU6050_DLPF_5HZ   = 0x06  /**< 带宽 5Hz / 5Hz, 延迟 18.6ms */
    } MPU6050_DLPFConfig;

    /** @brief 时钟源选择 */
    typedef enum
    {
        MPU6050_CLK_INTERNAL = 0x00, /**< 内部 8MHz 振荡器 */
        MPU6050_CLK_PLL_X    = 0x01, /**< PLL with X axis gyroscope reference */
        MPU6050_CLK_PLL_Y    = 0x02, /**< PLL with Y axis gyroscope reference */
        MPU6050_CLK_PLL_Z    = 0x03, /**< PLL with Z axis gyroscope reference */
        MPU6050_CLK_EXT32K   = 0x04, /**< 外部 32.768kHz 时钟 */
        MPU6050_CLK_EXT19M   = 0x05, /**< 外部 19.2MHz 时钟 */
        MPU6050_CLK_STOP     = 0x07  /**< 停止时钟 */
    } MPU6050_ClockSource;

    /** @brief 中断事件源 */
    typedef enum
    {
        MPU6050_INT_DATA_RDY    = (1U << 0), /**< 数据就绪 */
        MPU6050_INT_MOTION      = (1U << 2), /**< 运动检测 */
        MPU6050_INT_ZERO_MOTION = (1U << 3), /**< 静止检测 */
        MPU6050_INT_FREE_FALL   = (1U << 4)  /**< 自由落体检测 */
    } MPU6050_IntSource;

    /*---------------------------- 结构体定义 ---------------------------------*/
    /**
     * @brief MPU6050 初始化配置结构体
     */
    typedef struct
    {
        MPU6050_AccelFullScale AccelFullScale; /**< 加速度计量程 */
        MPU6050_GyroFullScale  GyroFullScale;  /**< 陀螺仪量程 */
        MPU6050_DLPFConfig     DLPFConfig;     /**< DLPF 配置 */
        MPU6050_ClockSource    ClockSource;    /**< 时钟源 */
        uint8_t                SampleRateDiv;  /**< 采样率分频器 (Sample Rate = Gyro Output Rate / (1+Div)) */
#if MPU6050_USE_INTERRUPTS
        uint8_t IntEnable;      /**< 中断使能掩码 (MPU6050_IntSource 组合) */
        uint8_t IntPinLevel;    /**< 0: 低有效, 1: 高有效 */
        uint8_t IntLatch;       /**< 0: 50us脉冲, 1: 锁存直到清除 */
        uint8_t IntClearOnRead; /**< 0: 仅读取状态寄存器清除, 1: 任何寄存器读取清除 */
#endif
    } MPU6050_ConfigTypeDef;

    /**
     * @brief MPU6050 设备句柄 (支持多实例)
     */
    typedef struct
    {
        uint8_t               DeviceAddr; /**< I2C 设备地址 */
        MPU6050_I2C_ReadFunc  I2C_Read;   /**< I2C 读函数指针 */
        MPU6050_I2C_WriteFunc I2C_Write;  /**< I2C 写函数指针 */
        MPU6050_DelayMsFunc   DelayMs;    /**< 毫秒延时函数指针 */

        /* 内部状态 */
        uint8_t IsInit; /**< 初始化完成标志 */

        /* 满量程与换算系数 (内部使用) */
        float AccelLSB; /**< 加速度 LSB/g 换算值 */
        float GyroLSB;  /**< 陀螺仪 LSB/(°/s) 换算值 */

#if MPU6050_USE_CALIBRATION
        int16_t AccelBias[3]; /**< 加速度计零偏 (原始ADC) */
        int16_t GyroBias[3];  /**< 陀螺仪零偏 (原始ADC) */
#endif

#if MPU6050_USE_DMP
        uint8_t DmpEnabled; /**< DMP 是否已启动 */
#endif
    } MPU6050_HandleTypeDef;

    /*------------------------- 全局函数声明 --------------------------------*/

    /*-------- 硬件抽象注册（也可在初始化前直接设置句柄成员）--------*/
    /**
     * @brief 向句柄注册 I2C 读函数
     * @param hmpu 设备句柄指针
     * @param read_func I2C 读函数指针
     * @return MPU6050_OK
     */
    MPU6050_Status MPU6050_RegisterI2CReadFunc(MPU6050_HandleTypeDef *hmpu, MPU6050_I2C_ReadFunc read_func);

    /**
     * @brief 向句柄注册 I2C 写函数
     * @param hmpu 设备句柄指针
     * @param write_func I2C 写函数指针
     * @return MPU6050_OK
     */
    MPU6050_Status MPU6050_RegisterI2CWriteFunc(MPU6050_HandleTypeDef *hmpu, MPU6050_I2C_WriteFunc write_func);

    /**
     * @brief 向句柄注册延时函数
     * @param hmpu 设备句柄指针
     * @param delay_func 延时函数指针
     * @return MPU6050_OK
     */
    MPU6050_Status MPU6050_RegisterDelayFunc(MPU6050_HandleTypeDef *hmpu, MPU6050_DelayMsFunc delay_func);

    /*-------- 初始化和基本配置 --------*/
    /**
     * @brief 初始化 MPU6050 设备
     * @param hmpu 设备句柄指针 (需提前填充 I2C 函数指针和地址)
     * @param config 初始化配置 (可为 NULL 使用默认值)
     * @return 错误码
     */
    MPU6050_Status MPU6050_Init(MPU6050_HandleTypeDef *hmpu, const MPU6050_ConfigTypeDef *config);

    /**
     * @brief 复位设备（包括所有寄存器和 FIFO）
     * @param hmpu 设备句柄指针
     * @return 错误码
     */
    MPU6050_Status MPU6050_Reset(MPU6050_HandleTypeDef *hmpu);

    /**
     * @brief 唤醒设备 (清除睡眠模式)
     * @param hmpu 设备句柄指针
     * @return 错误码
     */
    MPU6050_Status MPU6050_WakeUp(MPU6050_HandleTypeDef *hmpu);

    /*-------- 数据读取与转换 --------*/
    /**
     * @brief 读取加速度原始 ADC 值
     * @param hmpu 设备句柄
     * @param accel_adc 存储三轴加速度值 (数组长度至少3)
     * @return 错误码
     */
    MPU6050_Status MPU6050_GetAccelRaw(MPU6050_HandleTypeDef *hmpu, int16_t accel_adc[3]);

    /**
     * @brief 读取加速度物理值 (g)
     * @param hmpu 设备句柄
     * @param accel_g 存储加速度值 (g, 数组长度至少3)
     * @return 错误码
     * @note 若启用校准，会自动扣除零偏。
     */
    MPU6050_Status MPU6050_GetAccelData(MPU6050_HandleTypeDef *hmpu, float accel_g[3]);

    /**
     * @brief 读取陀螺仪原始 ADC 值
     * @param hmpu 设备句柄
     * @param gyro_adc 存储三轴角速度值 (数组长度至少3)
     * @return 错误码
     */
    MPU6050_Status MPU6050_GetGyroRaw(MPU6050_HandleTypeDef *hmpu, int16_t gyro_adc[3]);

    /**
     * @brief 读取陀螺仪物理值 (°/s)
     * @param hmpu 设备句柄
     * @param gyro_dps 存储角速度值 (°/s, 数组长度至少3)
     * @return 错误码
     * @note 若启用校准，会自动扣除零偏。
     */
    MPU6050_Status MPU6050_GetGyroData(MPU6050_HandleTypeDef *hmpu, float gyro_dps[3]);

    /**
     * @brief 读取温度 (摄氏度)
     * @param hmpu 设备句柄
     * @param temp_c 温度输出
     * @return 错误码
     */
    MPU6050_Status MPU6050_GetTemperature(MPU6050_HandleTypeDef *hmpu, float *temp_c);

#if MPU6050_USE_SELF_TEST
    /*-------- 自检 --------*/
    /**
     * @brief 执行 MPU6050 自检并计算偏差百分比
     * @param hmpu 设备句柄
     * @param accel_bias_pct 加速度偏差百分比输出 (数组至少3, %)
     * @param gyro_bias_pct  陀螺仪偏差百分比输出 (数组至少3, %)
     * @return MPU6050_OK 或 MPU6050_ERR_SELFTEST
     */
    MPU6050_Status MPU6050_SelfTest(MPU6050_HandleTypeDef *hmpu, float accel_bias_pct[3], float gyro_bias_pct[3]);
#endif

#if MPU6050_USE_CALIBRATION
    /*-------- 零偏校准 --------*/
    /**
     * @brief 自动静态校准（设备需静止）
     * @param hmpu   设备句柄
     * @param samples 采样次数 (建议 100-200)
     * @return 错误码
     */
    MPU6050_Status MPU6050_Calibrate(MPU6050_HandleTypeDef *hmpu, uint16_t samples);

    /**
     * @brief 将校准偏置清零
     * @param hmpu 设备句柄
     */
    void MPU6050_ResetCalibration(MPU6050_HandleTypeDef *hmpu);
#endif

#if MPU6050_USE_INTERRUPTS
    /*-------- 中断配置 --------*/
    /**
     * @brief 配置中断源并使能中断
     * @param hmpu 设备句柄
     * @param int_mask 中断源掩码 (MPU6050_IntSource 组合)
     * @note  具体用法：
     *        只使能数据就绪中断:              MPU6050_SetInterrupts(&mpu, MPU6050_INT_DATA_RDY);
     *        同时使能运动检测和自由落体中断:   MPU6050_SetInterrupts(&mpu, MPU6050_INT_MOTION | MPU6050_INT_FREE_FALL);
     *        如果您传入0，则所有中断都会被禁止，所以它不会一下子启动所有中断，而是精确控制哪些中断被激活
     * @return 错误码
     */
    MPU6050_Status MPU6050_SetInterrupts(MPU6050_HandleTypeDef *hmpu, uint8_t int_mask);

    /**
     * @brief 获取当前中断状态
     * @param hmpu 设备句柄
     * @param status 中断状态返回 (1表示发生)
     * @return 错误码
     */
    MPU6050_Status MPU6050_GetIntStatus(MPU6050_HandleTypeDef *hmpu, uint8_t *status);

    /**
     * @brief 设置运动检测阈值和持续时间
     * @param hmpu 设备句柄
     * @param threshold 阈值 (约 0~255, 1LSB=2mg)
     * @param duration 持续时间 (1ms/LSB)
     * @return 错误码
     */
    MPU6050_Status MPU6050_SetMotionDetection(MPU6050_HandleTypeDef *hmpu, uint8_t threshold, uint8_t duration);

    /**
     * @brief 设置零运动检测阈值和持续时间
     * @param hmpu 设备句柄
     * @param threshold 阈值
     * @param duration 持续时间
     * @return 错误码
     */
    MPU6050_Status MPU6050_SetZeroMotionDetection(MPU6050_HandleTypeDef *hmpu, uint8_t threshold, uint8_t duration);

    /**
     * @brief 设置自由落体检测阈值和持续时间
     * @param hmpu 设备句柄
     * @param threshold 阈值
     * @param duration 持续时间
     * @return 错误码
     */
    MPU6050_Status MPU6050_SetFreeFallDetection(MPU6050_HandleTypeDef *hmpu, uint8_t threshold, uint8_t duration);
#endif

#if MPU6050_USE_LOW_POWER
    /*-------- 低功耗管理 --------*/
    /**
     * @brief 进入睡眠模式
     * @param hmpu 设备句柄
     * @return 错误码
     */
    MPU6050_Status MPU6050_EnterSleep(MPU6050_HandleTypeDef *hmpu);

    /**
     * @brief 配置循环模式 (Cycle Mode) 加速度计低功耗
     * @param hmpu 设备句柄
     * @param awake_freq 唤醒频率 (1-4: 1.25/5/20/40Hz)
     * @return 错误码
     */
    MPU6050_Status MPU6050_SetCycleMode(MPU6050_HandleTypeDef *hmpu, uint8_t awake_freq);
#endif

#if MPU6050_USE_FIFO
    /*-------- FIFO 操作 --------*/
    /**
     * @brief 配置 FIFO 存储内容
     * @param hmpu 设备句柄
     * @param accel_en 存储加速度 (1:使能,0:禁止)
     * @param gyro_en  存储陀螺仪
     * @param temp_en  存储温度
     * @return 错误码
     */
    MPU6050_Status MPU6050_SetFIFOConfig(MPU6050_HandleTypeDef *hmpu, uint8_t accel_en, uint8_t gyro_en,
                                         uint8_t temp_en);

    /**
     * @brief 读取当前 FIFO 中的字节数
     * @param hmpu 设备句柄
     * @param count 输出字节数
     * @return 错误码
     */
    MPU6050_Status MPU6050_GetFIFOCount(MPU6050_HandleTypeDef *hmpu, uint16_t *count);

    /**
     * @brief 从 FIFO 读取指定长度数据
     * @param hmpu 设备句柄
     * @param buf 缓冲区
     * @param len 读取长度 (不超过 FIFO 现有数据)
     * @return 错误码 (若溢出会返回 MPU6050_ERR_FIFO_OVF)
     */
    MPU6050_Status MPU6050_ReadFIFO(MPU6050_HandleTypeDef *hmpu, uint8_t *buf, uint16_t len);

    /**
     * @brief 复位 FIFO
     * @param hmpu 设备句柄
     * @return 错误码
     */
    MPU6050_Status MPU6050_ResetFIFO(MPU6050_HandleTypeDef *hmpu);
#endif

#if MPU6050_USE_DMP
    /*-------- DMP 数字运动处理器 (仅接口框架) --------*/
    /**
     * @brief DMP 初始化并启动
     * @note  需要用户提供官方 DMP 固件数组 (见 mpu6050.c 中说明)
     * @param hmpu 设备句柄
     * @return 错误码
     */
    MPU6050_Status MPU6050_DMP_Init(MPU6050_HandleTypeDef *hmpu);

    /**
     * @brief 获取 DMP 输出的四元数 (需启用 DMP)
     * @param hmpu 设备句柄
     * @param q 四元数 [w, x, y, z] (浮点)
     * @return 错误码
     */
    MPU6050_Status MPU6050_DMP_GetQuaternion(MPU6050_HandleTypeDef *hmpu, float q[4]);

    /**
     * @brief 获取 DMP 计算的欧拉角 (Z-Y-X 顺序, 度)
     * @param hmpu 设备句柄
     * @param yaw   偏航角 (°)
     * @param pitch 俯仰角 (°)
     * @param roll  横滚角 (°)
     * @return 错误码
     */
    MPU6050_Status MPU6050_DMP_GetEuler(MPU6050_HandleTypeDef *hmpu, float *yaw, float *pitch, float *roll);

    /**
     * @brief 通过 DMP 获取处理后的加速度 (重力坐标系)
     * @param hmpu 设备句柄
     * @param accel 加速度数组 (g)
     * @return 错误码
     */
    MPU6050_Status MPU6050_DMP_GetAccel(MPU6050_HandleTypeDef *hmpu, float accel[3]);

    /**
     * @brief 通过 DMP 获取处理后的陀螺仪 (体坐标系)
     * @param hmpu 设备句柄
     * @param gyro 角速度数组 (°/s)
     * @return 错误码
     */
    MPU6050_Status MPU6050_DMP_GetGyro(MPU6050_HandleTypeDef *hmpu, float gyro[3]);
#endif

#if MPU6050_USE_AUX_I2C
    /*-------- 辅助 I2C 接口 --------*/
    /**
     * @brief 使能/禁用辅助 I2C 直通模式 (Bypass) 以访问外部磁力计
     * @param hmpu 设备句柄
     * @param enable 1:使能直通, 0:禁用
     * @return 错误码
     */
    MPU6050_Status MPU6050_SetBypassMode(MPU6050_HandleTypeDef *hmpu, uint8_t enable);
#endif

/*---------------------------- 寄存器定义 ---------------------------------*/
/* 以下为 MPU6050 常用寄存器地址（内部使用，不依赖魔数） */
#define MPU6050_REG_WHO_AM_I      0x75U
#define MPU6050_REG_USER_CTRL     0x6AU
#define MPU6050_REG_PWR_MGMT_1    0x6BU
#define MPU6050_REG_PWR_MGMT_2    0x6CU
#define MPU6050_REG_SMPLRT_DIV    0x19U
#define MPU6050_REG_CONFIG        0x1AU
#define MPU6050_REG_GYRO_CONFIG   0x1BU
#define MPU6050_REG_ACCEL_CONFIG  0x1CU
#define MPU6050_REG_ACCEL_CONFIG2 0x1DU
#define MPU6050_REG_FIFO_EN       0x23U
#define MPU6050_REG_INT_PIN_CFG   0x37U
#define MPU6050_REG_INT_ENABLE    0x38U
#define MPU6050_REG_INT_STATUS    0x3AU
#define MPU6050_REG_ACCEL_XOUT_H  0x3BU
#define MPU6050_REG_TEMP_OUT_H    0x41U
#define MPU6050_REG_GYRO_XOUT_H   0x43U
#define MPU6050_REG_SELF_TEST_X   0x0DU
#define MPU6050_REG_SELF_TEST_Y   0x0EU
#define MPU6050_REG_SELF_TEST_Z   0x0FU
#define MPU6050_REG_SELF_TEST_A   0x10U
#define MPU6050_REG_MOT_THR       0x1FU
#define MPU6050_REG_MOT_DUR       0x20U
#define MPU6050_REG_ZRMOT_THR     0x21U
#define MPU6050_REG_ZRMOT_DUR     0x22U
#define MPU6050_REG_FF_THR        0x1DU
#define MPU6050_REG_FF_DUR        0x1EU
#define MPU6050_REG_FIFO_COUNTH   0x72U
#define MPU6050_REG_FIFO_R_W      0x74U

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H__ */

/**
 * @example 使用示例 (位于 main.c 或其他源文件)
 * @code
 * #include "mpu6050.h"
 *
 * // 用户需实现的 I2C 读写及延时函数，例如：
 * // int32_t my_i2c_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len) {...}
 * // int32_t my_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *buf, uint8_t len) {...}
 * // void my_delay_ms(uint32_t ms) {...}
 *
 * int main(void) {
 *     MPU6050_HandleTypeDef mpu;
 *     MPU6050_ConfigTypeDef config;
 *     float accel[3], gyro[3];
 *
 *     // 填充句柄
 *     mpu.DeviceAddr = MPU6050_DEFAULT_DEVICE_ADDR;
 *     mpu.I2C_Read = my_i2c_read;
 *     mpu.I2C_Write = my_i2c_write;
 *     mpu.DelayMs = my_delay_ms;
 *
 *     // 可选：使用注册函数
 *     // MPU6050_RegisterI2CReadFunc(&mpu, my_i2c_read);
 *
 *     // 设置默认配置
 *     config.AccelFullScale = MPU6050_ACCEL_FS_4G;
 *     config.GyroFullScale  = MPU6050_GYRO_FS_1000;
 *     config.DLPFConfig     = MPU6050_DLPF_42HZ;
 *     config.ClockSource    = MPU6050_CLK_PLL_X;
 *     config.SampleRateDiv  = 0;
 *     // ... 中断等配置略
 *
 *     if (MPU6050_Init(&mpu, &config) != MPU6050_OK) {
 *         // 错误处理
 *     }
 *
 *     // 可选校准 (设备需保持水平静止)
 *     #if MPU6050_USE_CALIBRATION
 *     MPU6050_Calibrate(&mpu, 200);
 *     #endif
 *
 *     while (1) {
 *         MPU6050_GetAccelData(&mpu, accel);
 *         MPU6050_GetGyroData(&mpu, gyro);
 *         // 使用 accel[0], accel[1], accel[2] 和 gyro[0], gyro[1], gyro[2]
 *         // 延时...
 *     }
 * }
 * @endcode
 */
