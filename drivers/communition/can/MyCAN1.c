// MyCAN.c
/*
*author RanXin
*time 2026.5.23
*该文件以轮询方式完成CAN的收发。
*/
#include "can.h"
#include "MyCAN1.h"
#include "stm32f1xx_hal.h"
/*
* 前言：该代码设置的配置为
*  hcan.Init.SyncJumpWidth = CAN_SJW_2TQ;
*  hcan.Init.TimeSeg1 = CAN_BS1_2TQ;
*  hcan.Init.TimeSeg2 = CAN_BS2_3TQ;
*  can波特率为125K，时钟72，预分频器为48。
*/

/*
*@brief 初始化CAN模块，配置过滤器，并启动CAN。
*/
void MyCAN_Init(void)
{
//    // 1. 使能 CAN 时钟（如果 MX_CAN_Init 已使能，此处可省略，但为了安全保留）
//    __HAL_RCC_CAN1_CLK_ENABLE();

//    // 2. 配置 CAN 参数（回环模式，125kbps，BS1=2TQ，BS2=3TQ，SJW=2TQ）
//    hcan.Instance = CAN1;
//    hcan.Init.Prescaler = 48;          // 假设 APB1 = 36MHz，得到 125kbps
//    hcan.Init.Mode = CAN_MODE_NORMAL; // 正常模式
//    hcan.Init.SyncJumpWidth = CAN_SJW_2TQ;
//    hcan.Init.TimeSeg1 = CAN_BS1_2TQ;
//    hcan.Init.TimeSeg2 = CAN_BS2_3TQ;
//    hcan.Init.TimeTriggeredMode = DISABLE;
//    hcan.Init.AutoBusOff = DISABLE;
//    hcan.Init.AutoWakeUp = DISABLE;
//    hcan.Init.AutoRetransmission = DISABLE;
//    hcan.Init.ReceiveFifoLocked = DISABLE;
//    hcan.Init.TransmitFifoPriority = DISABLE;

//    if (HAL_CAN_Init(&hcan) != HAL_OK) {
//        Error_Handler();
//    }

    // 3. 配置过滤器（全通，接收所有报文）
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14; // 对于单 CAN，此值无影响，但习惯填 14

    if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK) {
        Error_Handler();
    }

    // 4. 启动 CAN
    if (HAL_CAN_Start(&hcan) != HAL_OK) {
        Error_Handler();
    }
}

/*
*@brief 发送 CAN 报文
*@param ID: 报文 ID (11 位标准帧或 29 位扩展帧)
*@param Length: 数据长度 (0-8)
*@param Data: 数据指针，长度由 Length 指定
*@retval None 
*/
void MyCAN_Transmit(uint32_t ID, uint8_t Length, uint8_t *Data)
{
    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox;

    tx_header.StdId = ID;
    tx_header.ExtId = ID;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = Length;
    tx_header.TransmitGlobalTime = DISABLE;

    // 发送数据
    if (HAL_CAN_AddTxMessage(&hcan, &tx_header, Data, &tx_mailbox) != HAL_OK)
    {
        // 发送失败，可处理
        return;
    }

    // 等待发送完成 (超时保护)
    uint32_t timeout = 0;
    while (HAL_CAN_IsTxMessagePending(&hcan, tx_mailbox))
    {
        timeout++;
        if (timeout > 100000)
            break;
    }
}

/*
*@brief 检查是否有待接收的 CAN 报文
*@retval 1: 有报文待接收, 0: 无报文待接收
*/
uint8_t MyCAN_ReceiveFlag(void)
{
    // 检查 FIFO0 中是否有待接收的报文
    return (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0) ? 1 : 0;
}

/*
*@brief 从 CAN 接收报文
*@param ID: 输出参数，接收报文的 ID
*@param Length: 输出参数，接收报文的数据长度
*@param Data: 输出参数，接收报文的数据指针，长度由 Length 指定
*@retval None
*/
void MyCAN_Receive(uint32_t *ID, uint8_t *Length, uint8_t *Data)
{
    CAN_RxHeaderTypeDef rx_header;

    // 从 FIFO0 接收报文
    if (HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rx_header, Data) != HAL_OK)
    {
        return;
    }

    // 获取 ID
    if (rx_header.IDE == CAN_ID_STD)
        *ID = rx_header.StdId;
    else
        *ID = rx_header.ExtId;

    // 获取数据长度 (仅对数据帧)
    if (rx_header.RTR == CAN_RTR_DATA)
        *Length = rx_header.DLC;
    else
        *Length = 0;  // 远程帧无数据
}
