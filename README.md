# MCU Modules - 通用单片机模块驱动库

仓库目的是建立一个高度可移植、可拓展的、供大学生使用的单片机外设驱动模板库
支持STM32等主流平台

## ✨ 特性
- 严格的分层架构，驱动与硬件完全解耦
- 统一的API接口，所有驱动使用方式一致
- 支持多实例，可同时驱动多个相同外设
- 编译时裁剪，按需启用功能
- 完整的示例代码和文档

## 🚀 快速开始
1. 克隆仓库：`git clone https://github.com/yourname/mcu-modules.git`
2. 将`drivers`、`port`、`utils`目录添加到你的工程
3. 复制`port/your_platform/port_config.h`到工程目录并修改配置
4. 包含对应驱动的头文件：`#include "ssd1306.h"`
5. 参考`examples`目录中的示例代码使用

## 📁 仓库结构 # mcu-modules



mcu-modules/
├── 📄 README.md                    # 仓库总览、快速开始、贡献指南  
├── 📄 LICENSE                      # 开源许可证（推荐MIT）  
├── 📄 CHANGELOG.md                 # 版本更新日志  
├── 📄 .gitignore                   # Git忽略文件  
├── 📄 .clang-format                # 代码格式化规范  
├── 📄 Doxyfile                     # 自动生成文档配置  
│  
├── 📁 algos/                        # 详细文档目录  
│   ├── 📁 smr  
│   ├── 📁 pid/  
│   └── 📁 lqr/  
│  
├── 📁 docs/                        # 详细文档目录  
│   ├── 📄 getting-started.md       # 快速上手教程  
│   ├── 📄 porting-guide.md         # 移植到新芯片指南  
│   ├── 📄 coding-style.md          # 代码风格规范  
│   ├── 📄 contribution.md          # 贡献指南  
│   ├── 📁 api/                     # 各模块API文档  
│   └── 📁 images/                  # 文档配图  
│  
├── 📁 drivers/                     # 核心驱动层（与硬件无关）  
│   ├── 📄 README.md                # 驱动层说明  
│   ├── 📁 display/                 # 显示类驱动  
│   │   ├── oled_ssd1306/           # SSD1306 OLED驱动  
│   │   │   ├── 📄 ssd1306.c  
│   │   │   ├── 📄 ssd1306.h  
│   │   │   └── 📄 README.md        # 该模块单独说明  
│   │   ├── lcd_st7735/  
│   │   └── lcd_ili9341/  
│   ├── 📁 sensor/                  # 传感器类驱动  
│   │   ├── dht11/  
│   │   ├── bmp280/  
│   │   ├── mpu6050/  
│   │   └── hx711/  
│   ├── 📁 communication/           # 通信类驱动  
│   │   ├── uart/  
│   │   ├── i2c/  
│   │   ├── spi/  
│   │   └── can/  
│   ├── 📁 input/                   # 输入类驱动  
│   │   ├── key/  
│   │   ├── encoder/  
│   │   └── touch/  
│   ├── 📁 output/                  # 输出类驱动  
│   │   ├── led/  
│   │   ├── buzzer/  
│   │   ├── relay/  
│   │   └── pwm/  
│   └── 📁 storage/                 # 存储类驱动  
│       ├── eeprom/  
│       ├── flash/  
│       └── sd_card/  
│  
├── 📁 port/                        # 平台移植层（唯一与硬件相关的部分）  
│   ├── 📄 README.md                # 移植层说明  
│   ├── 📁 stm32f1xx_hal/           # STM32F1系列HAL库移植  
│   │   ├── 📄 mcu_port.c  
│   │   ├── 📄 mcu_port.h  
│   │   └── 📄 port_config.h        # 移植配置文件  
│   ├── 📁 stm32f4xx_hal/  
│   ├── 📁 stm32h7xx_hal/  
│   ├── 📁 gd32f1xx_hal/  
│   └── 📁 esp32_idf/  
│  
├── 📁 utils/                       # 通用工具函数库（完全硬件无关）  
│   ├── 📄 ring_buffer.c            # 环形缓冲区  
│   ├── 📄 ring_buffer.h  
│   ├── 📄 fifo.c                   # FIFO队列  
│   ├── 📄 fifo.h  
│   ├── 📄 crc.c                    # CRC校验  
│   ├── 📄 crc.h  
│   ├── 📄 delay.c                  # 通用延时（依赖port层）  
│   └── 📄 delay.h  
│  
├── 📁 examples/                    # 示例工程目录  
│   ├── 📄 README.md                # 示例说明  
│   ├── 📁 stm32f103c8t6/           # STM32F103C8T6示例  
│   │   ├── 📁 oled_ssd1306_demo/  
│   │   ├── 📁 mpu6050_demo/  
│   │   └── 📁 all_modules_demo/    # 所有模块综合演示  
│   ├── 📁 stm32f407vet6/  
│   └── 📁 esp32_devkitc_v4/  
│  
├── 📁 templates/                   # 空白模板目录  
│   ├── 📄 driver_template.c        # 新驱动模板  
│   ├── 📄 driver_template.h  
│   └── 📄 port_template.c          # 新平台移植模板  
│  
└── 📁 tests/                       # 单元测试目录  
    ├── 📄 README.md  
    ├── 📁 unit/                    # 单元测试  
    └── 📁 integration/             # 集成测试  
