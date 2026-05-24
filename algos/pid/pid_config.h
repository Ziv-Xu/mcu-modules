// pid_config.h
#ifndef PID_CONFIG_H
#define PID_CONFIG_H

// 以 ARM Cortex-M 为例
#include "stm32f1xx.h" // 或你的芯片头文件，提供 __disable_irq 等
// 用于需要中断中调用 PID 的场景，确保计算过程不被打断，避免数据不一致问题。
#define PID_ENTER_CRITICAL() __disable_irq()
#define PID_EXIT_CRITICAL()  __enable_irq()

#endif /* PID_CONFIG_H */
