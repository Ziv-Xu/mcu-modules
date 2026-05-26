# UART 驱动移植指南
## 概述
本驱动模板采用硬件抽象层（HAL）设计，移植到新 MCU 时只需完成 HAL 操作集的实现，无需修改核心逻辑。

## 移植步骤

### 1. 准备平台头文件
根据 MCU 定义必要的标志位，例如：
```c
#define UART_FLAG_TXE   (1 << 7)
#define UART_FLAG_TC    (1 << 6)
#define UART_FLAG_RXNE  (1 << 5)
#define UART_FLAG_IDLE  (1 << 4)
#define UART_FLAG_ORE   (1 << 3)
#define UART_FLAG_FE    (1 << 0)
#define UART_FLAG_PE    (1 << 1)
#define UART_FLAG_NE    (1 << 2)
```
