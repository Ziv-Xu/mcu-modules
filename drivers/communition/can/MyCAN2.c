/*
*author RanXin
*time 2026.5.23
*该文件为CAN的中断接收文件。
*/

#include "MyCAN2.h"
#include "can.h"
#include "stm32f1xx_hal_can.h"


CAN_RxHeaderTypeDef MyCAN_RxHeader; // 接收报文头结构体
uint8_t MyCAN_RxData[8];            // 接收数据缓冲区
uint8_t MyCAN_RxFlag = 0;           // 接收完成标志，1 表示有新数据

/* CAN 接收完成回调（FIFO0 有消息时自动调用）*/
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &MyCAN_RxHeader, MyCAN_RxData);
    MyCAN_RxFlag = 1;
}

/*
* CAN 初始化函数，配置波特率、过滤器等参数
*注意：CAN 波特率的计算公式为：速率 = PCLK1 / Prescaler / (1 + TimeSeg1 + TimeSeg2)
*其中，PCLK1 是 APB1 总线时钟频率，Prescaler 是预分频器，TimeSeg1 和 TimeSeg2 是时间段设置。
*波特率这些都可以在MX中设置，目前过滤器不知道如何通过MX设置，需要手动设置，此处设置为全通过滤器，接收所有报文。
*/
void MyCAN_Init(void)
{
    
/*
*该项由cubeMX来设置，主要是设置通信速率。
*速率=PCLK1/Prescaler/（1+TimeSeg1+TimeSeg2）
*/

//    // 1. 使能 GPIO 和 CAN 时钟（CubeMX 已做，但为安全再执行一次）
//    __HAL_RCC_GPIOA_CLK_ENABLE();
//    __HAL_RCC_CAN1_CLK_ENABLE();

//    // 2. 配置 PA11 (RX) 和 PA12 (TX)
//    GPIO_InitTypeDef gpio_init;
//    gpio_init.Pin = GPIO_PIN_12;
//    gpio_init.Mode = GPIO_MODE_AF_PP;
//    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
//    HAL_GPIO_Init(GPIOA, &gpio_init);

//    gpio_init.Pin = GPIO_PIN_11;
//    gpio_init.Mode = GPIO_MODE_INPUT;
//    gpio_init.Pull = GPIO_PULLUP;
//    HAL_GPIO_Init(GPIOA, &gpio_init);

//    // 3. CAN 参数配置
//    hcan.Instance = CAN1;
//    hcan.Init.Prescaler = 48;          // 36MHz /48 /(1+2+3)=125kbps
//    hcan.Init.Mode = CAN_MODE_LOOPBACK; // 回环模式，测试用；正常模式改为 CAN_MODE_NORMAL
//    hcan.Init.SyncJumpWidth = CAN_SJW_2TQ;
//    hcan.Init.TimeSeg1 = CAN_BS1_2TQ;
//    hcan.Init.TimeSeg2 = CAN_BS2_3TQ;
//    hcan.Init.TimeTriggeredMode = DISABLE;
//    hcan.Init.AutoBusOff = DISABLE;
//    hcan.Init.AutoWakeUp = DISABLE;
//    hcan.Init.AutoRetransmission = DISABLE;
//    hcan.Init.ReceiveFifoLocked = DISABLE;
//    hcan.Init.TransmitFifoPriority = DISABLE;
//    HAL_CAN_Init(&hcan);

    // 4. 配置过滤器（全通，接收所有报文）
    CAN_FilterTypeDef filter;
    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;
    HAL_CAN_ConfigFilter(&hcan, &filter);

    // 5. 启动 CAN 外设
    HAL_CAN_Start(&hcan);

    // 6. 激活接收中断（FIFO0 消息挂起中断）
    /*warning：接收数据所需开启的中断是 USB Low priority or CAN RX0 interrupts*/
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

    // 7. 设置 NVIC 优先级（可选，CubeMX 已配置时可省略）
//    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 1, 1);
//    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}

/*
*@brief CAN 发送函数，封装 HAL_CAN_AddTxMessage
*@param Id: 标识符，标准帧为 11 位，扩展帧为 29 位
*@param IdType: 标识符类型，CAN_ID_STD 或 CAN_ID_EXT
*@param pData: 数据指针，指向要发送的数据缓冲区
*@param DLC: 数据长度代码，表示数据字节数（0-8）理论上可以到达64
*@retval None 
*/
void MyCAN_Transmit(uint32_t Id, uint8_t IdType, uint8_t *pData, uint8_t DLC)
{
    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox;

    tx_header.IDE = IdType;          // CAN_ID_STD 或 CAN_ID_EXT
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = DLC;
    tx_header.TransmitGlobalTime = DISABLE;

    if (IdType == CAN_ID_STD) {
        tx_header.StdId = Id;
        tx_header.ExtId = 0;
    } else {
        tx_header.StdId = 0;
        tx_header.ExtId = Id;
    }

    if (HAL_CAN_AddTxMessage(&hcan, &tx_header, pData, &tx_mailbox) != HAL_OK) {
        return;
    }

    uint32_t timeout = 0;
    while (HAL_CAN_IsTxMessagePending(&hcan, tx_mailbox)) {
        timeout++;
        if (timeout > 100000) break;
    }
}
