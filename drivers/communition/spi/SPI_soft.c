
#include "stm32f1xx_hal.h"
#include "string.h"
#include "SPI_soft.h"
/**
 * @brief 向 SPI 总线发送一个字节（模拟 SPI，CPOL=0, CPHA=0）
 * @param data 待发送数据
 * @retval none
 */
void SPI_SendBit(uint8_t data)
{
    for (int i = 0; i < 8; i++)
    {
        SCL_L; // 时钟低电平
        if (data & 0x80)
            SDA_H;
        else
            SDA_L;
        SCL_H; // 时钟高电平，数据采样
        data <<= 1;
    }
}

/**
 * @brief 向 SPI 总线发送命令（寄存器地址）
 * @param reg 待发送命令
 * @retval none
 */
void SPI_SendIndex(uint8_t reg)
{
    CS_L;
    DC_L; // 命令模式
    SPI_SendBit(reg);
    CS_H;
}

/**
 * @brief 向SPI总线发送数据
 * @param data 待发送数据
 * @retval none
 */
void SPI_SendData(uint8_t Data)
{
    CS_L;
    DC_H; // 数据模式
    SPI_SendBit(Data);
    CS_H;
}

/**
 * @brief 发送命令 + 参数
 * @param adress 寄存器地址
 * @param data 待发送数据
 * @retval none
 */
void SPI_SendReg(uint8_t adress, uint8_t data)
{
    SPI_SendIndex(adress);
    SPI_SendData(data);
}

/**
 * @brief 硬件复位（RST 引脚拉低再拉高）
 * @retval none
 * @warning 在使用前，需要先运行一次，进行软件复位，已经写在Init函数里
 */
void SPI_Reset(void)
{
    RST_L;
    HAL_Delay(100);
    RST_H;
    HAL_Delay(50);
}

/**
 * @brief 读取一个字节（软件模拟，CPOL=0, CPHA=0）
 * @retval 读取到的字节
 * @note 需要连接 MISO 引脚，并在发送时钟时读取输入
 */
uint8_t SPI_ReadByte(void)
{
    uint8_t Byte = 0;
    for (int i = 0; i < 8; i++)
    {
        SCL_L;
        // 注意：SPI 读操作时，主机需要发送 0xFF 作为时钟（同时 MOSI 输出 0xFF 或者任意值）。
        // 这里为了支持读，我们也在 MOSI 上输出 0xFF（即 SDA_H），但也可以保持 SDA 不变。
        // 实际许多设备要求在读取数据期间 MOSI 保持高电平。
        SDA_H; // 主机输出高电平，提供时钟信号
        SCL_H;
        // 在时钟高电平期间采样 MISO 引脚
        Byte <<= 1;
        if (MISO_READ)
            Byte |= 1;
    }
    return Byte;
}

/**
 * @brief 读寄存器（先发送命令，再读数据）
 * @param reg 寄存器地址
 * @retval 读取到的寄存器值
 */
uint8_t SPI_ReadReg(uint8_t reg)
{
    uint8_t val;
    CS_L;
    // 发送命令（DC=0）
    DC_L;
    SPI_SendBit(reg);
    // 切换为读数据模式（DC=1）
    DC_H;
    val = SPI_ReadByte();
    CS_H;
    return val;
}
