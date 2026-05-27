/*
* ST7735 TFT 驱动函数
* 采用软件模拟SPI，目前是直接驱动寄存器，也可用HAL库的封装函数重写。
* 硬件暂未尝试。
*/

#include "stm32f1xx_hal.h"
#include "string.h"
#include "math.h"
#include "st7735TFT.h"
#include "font_asc.h"
#include "image1.h"


// 外部字模数组（由用户提供）
extern const unsigned char asc[];
extern const unsigned char chinese_font[];
extern const char font_sample[];


/**
 * @brief ST7735 引脚初始化（HAL 库方式）
 * @retval none
 */
void Spi_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 使能 GPIOB 和 GPIOA 时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // 配置 PB10, PB1, PB0 为推挽输出
    GPIO_InitStruct.Pin = BLK_PIN | DC_PIN | RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BDR_PORT, &GPIO_InitStruct);

    // 配置 PA5, PA7, PA4 为推挽输出
    GPIO_InitStruct.Pin = SCL_PIN | SDA_PIN | CS_PIN;
    HAL_GPIO_Init(CDS_PORT, &GPIO_InitStruct);
}

/**
 * @brief 向 SPI 总线发送一个字节（模拟 SPI，CPOL=0, CPHA=0）
 * @param data 待发送数据
 * @retval none
 */
void Spi_SendData(uint8_t data)
{
    for (int i = 0; i < 8; i++)
    {
        SCL_L;                         // 时钟低电平
        if (data & 0x80) SDA_H; else SDA_L;
        SCL_H;                         // 时钟高电平，数据采样
        data <<= 1;
    }
}

/**
 * @brief 向 ST7735 发送命令（寄存器地址）
 * @param reg 待发送命令
 * @retval none
 */
void TFT_SendIndex(uint8_t reg)
{
    CS_L;
    DC_L;                // 命令模式
    Spi_SendData(reg);
    CS_H;
}

/**
 * @brief 向 ST7735 发送数据
 * @param data 待发送数据
 * @retval none
 */
void TFT_SendData(uint8_t Data)
{
    CS_L;
    DC_H;                // 数据模式
    Spi_SendData(Data);
    CS_H;
}

/**
 * @brief 向 ST7735 发送 16 位数据（颜色）
 * @param Data 待发送数据
 * @retval none
 */
void TFT_Send16Bit(uint16_t Data)
{
    CS_L;
    DC_H;
    Spi_SendData(Data >> 8);
    Spi_SendData(Data);
    CS_H;
}

/**
 * @brief 发送命令 + 参数
 * @param adress 寄存器地址
 * @param data 待发送数据
 * @retval none
 */
void TFT_SendReg(uint8_t adress, uint8_t data)
{
    TFT_SendIndex(adress);
    TFT_SendData(data);
}

/**
 * @brief 硬件复位（RST 引脚拉低再拉高）
 * @retval none
 * @warning 在使用前，需要先运行一次，进行软件复位，已经写在Init函数里
 */
void TFT_Reset(void)
{
    RST_L;
    HAL_Delay(100);
    RST_H;
    HAL_Delay(50);
}

/**
 * @brief 显示屏初始化序列（与原代码完全相同）
 * @retval none
 */
void TFT_Init(void)
{
    Spi_Init();
    TFT_Reset();

    TFT_SendIndex(0x11);
    HAL_Delay(120);

    // 以下初始化序列与原代码一致，未做改动
    TFT_SendIndex(0x36); TFT_SendData(0x00);
    TFT_SendIndex(0x3A); TFT_SendData(0x05);
    TFT_SendIndex(0xB1); TFT_SendData(0x05); TFT_SendData(0x3C); TFT_SendData(0x3C);
    TFT_SendIndex(0xB2); TFT_SendData(0x05); TFT_SendData(0x3C); TFT_SendData(0x3C);
    TFT_SendIndex(0xB3); TFT_SendData(0x05); TFT_SendData(0x3C); TFT_SendData(0x3C);
    TFT_SendData(0x05); TFT_SendData(0x3C); TFT_SendData(0x3C);
    TFT_SendIndex(0xB4); TFT_SendData(0x03);
    TFT_SendIndex(0xC0); TFT_SendData(0x2E); TFT_SendData(0x06); TFT_SendData(0x04);
    TFT_SendIndex(0xC1); TFT_SendData(0xC0); TFT_SendData(0xC2);
    TFT_SendIndex(0xC2); TFT_SendData(0x0D); TFT_SendData(0x0D);
    TFT_SendIndex(0xC3); TFT_SendData(0x8D); TFT_SendData(0xEE);
    TFT_SendIndex(0xC4); TFT_SendData(0x8D); TFT_SendData(0xEE);
    TFT_SendIndex(0xC5); TFT_SendData(0x00);
    TFT_SendIndex(0x36); TFT_SendData(0xC0);
    TFT_SendIndex(0xe0); TFT_SendData(0x1B); TFT_SendData(0x21); TFT_SendData(0x10);
    TFT_SendData(0x15); TFT_SendData(0x2B); TFT_SendData(0x25); TFT_SendData(0x1F);
    TFT_SendData(0x23); TFT_SendData(0x22); TFT_SendData(0x22); TFT_SendData(0x2B);
    TFT_SendData(0x37); TFT_SendData(0x00); TFT_SendData(0x15); TFT_SendData(0x02);
    TFT_SendData(0x3F);
    TFT_SendIndex(0xE1); TFT_SendData(0x1A); TFT_SendData(0x20); TFT_SendData(0x0F);
    TFT_SendData(0x15); TFT_SendData(0x2A); TFT_SendData(0x25); TFT_SendData(0x1E);
    TFT_SendData(0x23); TFT_SendData(0x23); TFT_SendData(0x22); TFT_SendData(0x2B);
    TFT_SendData(0x37); TFT_SendData(0x00); TFT_SendData(0x15); TFT_SendData(0x02);
    TFT_SendData(0x3F);
    TFT_SendIndex(0x2C);
    TFT_SendIndex(0x29);
    TFT_Clear(BLACK);
}

/**
 * @brief 设置屏幕旋转方向
 * @param locate 旋转方向，0-3 分别对应 0°、90°、180°、270°
 * @retval none
 */
void TFt_SpinScreen(uint8_t locate)
{
    TFT_SendIndex(0x36);
    switch (locate)
    {
        case 0: TFT_SendData(0xC0); break;
        case 1: TFT_SendData(0xA0); break;
        case 2: TFT_SendData(0x00); break;
        case 3: TFT_SendData(0x60); break;
    }
}

/**
 * @brief 背光控制（需要连接 BLK 引脚）
 * @param io 0 关闭背光，1 打开背光
 * @retval none
 * @warning 需要连接 BLK 引脚才能控制背光，否则该函数无效
 */
void TFT_TurnOff(uint8_t io)
{
    if (io) BLK_H;
    else    BLK_L;
}

/**
 * @brief 设置显示区域（列地址+行地址）
 * @param x_start 起始列地址
 * @param y_start 起始行地址
 * @param x_end 结束列地址
 * @param y_end 结束行地址
 * @retval none
 */
void TFT_SetRegion(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    TFT_SendIndex(0x2a);
    TFT_SendData(0x00); TFT_SendData(x_start+2);
    TFT_SendData(0x00); TFT_SendData(x_end);
    TFT_SendIndex(0x2b);
    TFT_SendData(0x00); TFT_SendData(y_start+1);
    TFT_SendData(0x00); TFT_SendData(y_end);
    TFT_SendIndex(0x2c);
}

/**
 * @brief 全屏清屏
 * @param color 清屏颜色 (包括的颜色可以在.h文件里自行查看)
 * @retval none
 */
void TFT_Clear(uint16_t color)
{
    TFT_SetRegion(0, 0, 129, 160);
    TFT_SendIndex(0x2C);
    for (int i = 0; i < 128; i++)
        for (int m = 0; m < 160; m++)
            TFT_Send16Bit(color);
}

/**
 * @brief 区域填充
 * @param x1 起始列地址
 * @param y1 起始行地址
 * @param x2 结束列地址
 * @param y2 结束行地址
 * @param color 填充颜色
 * @retval none
 */
void TFT_FullScreen(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    TFT_SetRegion(x1, y1, x2, y2);
    int t = (x2 - x1 + 1) * (y2 - y1 + 1);
    while (t--) TFT_Send16Bit(color);
    TFT_SetRegion(0, 0, 129, 160);   // 恢复全屏区域
}

/**
 * @brief 设置光标位置（单点区域）
 */
void TFT_SetCursor(uint16_t x, uint16_t y)
{
    TFT_SetRegion(x, y, x, y);
}

/**
 * @brief 画一个点
 */
void TFT_DrawPoint(uint16_t x, uint16_t y, uint16_t Data)
{
    TFT_SetCursor(x, y);
    TFT_Send16Bit(Data);
}

/**
 * @brief 画圆（Bresenham 算法）
 */
void TFT_DrawCircle(uint16_t X, uint16_t Y, uint16_t R, uint16_t fc)
{
   {//Bresenham算法 
    unsigned short  a=0,b=R; 
    int c=3-2*R; 
    while (a<b){ 
        TFT_DrawPoint(X+a,Y+b,fc);     //        7 
        TFT_DrawPoint(X-a,Y+b,fc);     //        6 
        TFT_DrawPoint(X+a,Y-b,fc);     //        2 
        TFT_DrawPoint(X-a,Y-b,fc);     //        3 
        TFT_DrawPoint(X+b,Y+a,fc);     //        8 
        TFT_DrawPoint(X-b,Y+a,fc);     //        5 
        TFT_DrawPoint(X+b,Y-a,fc);     //        1 
        TFT_DrawPoint(X-b,Y-a,fc);     //        4 

        if(c<0) c=c+4*a+6; 
        else{ 
            c=c+4*(a-b)+10; 
            b-=1; 
        } 
       a+=1; 
    }if (a==b){ 
        TFT_DrawPoint(X+a,Y+b,fc); 
        TFT_DrawPoint(X+a,Y+b,fc); 
        TFT_DrawPoint(X+a,Y-b,fc); 
        TFT_DrawPoint(X-a,Y-b,fc); 
        TFT_DrawPoint(X+b,Y+a,fc); 
        TFT_DrawPoint(X-b,Y+a,fc); 
        TFT_DrawPoint(X+b,Y-a,fc); 
        TFT_DrawPoint(X-b,Y-a,fc); 
    } 
  } 
}

/**
 * @brief 画线（Bresenham 算法）
 */
void TFT_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t Color)
{
    int dx,            // difference in x's
    dy,             // difference in y's
    dx2,            // dx,dy * 2
    dy2, 
    x_inc,          // amount in pixel space to move during drawing
    y_inc,          // amount in pixel space to move during drawing
    error,          // the discriminant i.e. error i.e. decision variable
    index;          // used for looping	

	TFT_SetCursor(x0,y0);
	dx = x1-x0;//计算x距离
	dy = y1-y0;//计算y距离

	if (dx>=0) x_inc = 1; 
	else{
		x_inc = -1;
		dx    = -dx;  
	} 
	
	if (dy>=0) y_inc = 1; 
	else{
		y_inc = -1;
		dy    = -dy; 
	} 
	dx2 = dx << 1;
	dy2 = dy << 1;
	if (dx > dy) //x距离大于y距离，那么每个x轴上只有一个点，每个y轴上有若干个点
	{           //且线的点数等于x距离，以x轴递增画点
		           // initialize error term
		error = dy2 - dx; 
		// draw the line
		for (index=0; index <= dx; index++){ //要画的点数不会超过x距离{
			//画点
			TFT_DrawPoint(x0,y0,Color);
			
			// test if error has overflowed
			if (error >= 0) //是否需要增加y坐标值
			{
				error-=dx2;

				// move to next line
				y0+=y_inc;//增加y坐标值
			} // end if error overflowed

			// adjust the error term
			error+=dy2;

			// move to the next pixel
			x0+=x_inc;//x坐标值每次画点后都递增1
		} // end for
	} // end if |slope| <= 1
	else//y轴大于x轴，则每个y轴上只有一个点，x轴若干个点
	{//以y轴为递增画点
		// initialize error term
		error = dx2 - dy; 

		// draw the line
		for (index=0; index <= dy; index++)
		{
			// set the pixel
			TFT_DrawPoint(x0,y0,Color);

			// test if error overflowed
			if (error >= 0){
				error-=dy2;

				// move to next line
				x0+=x_inc;
			} // end if error overflowed

			// adjust the error term
			error+=dx2;

			// move to the next pixel
			y0+=y_inc;
		} // end for
	} // end else |slope| > 1
}
/**
 * @brief 画矩形框
 * @param x 矩形左上角 x 坐标
 * @param y 矩形左上角 y 坐标
 * @param w 矩形宽度
 * @param h 矩形高度
 * @param bc 矩形边框颜色
 */
void TFT_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t bc)  
{
    TFT_DrawLine(x, y, x+w, y, bc);
    TFT_DrawLine(x+w, y, x+w, y+h, bc);
    TFT_DrawLine(x, y+h, x+w, y+h, bc);
    TFT_DrawLine(x, y, x, y+h, bc);
}

/**
 * @brief 画矩形框（预设配色）
    * @param x 矩形左上角 x 坐标
    * @param y 矩形左上角 y 坐标
    * @param w 矩形宽度
    * @param h 矩形高度
    * @param mode 预设配色模式，0-3 分别对应绿灰、红黄、白灰、白灰2
 */
void TFT_box2(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t mode)
{
    if (mode==1)	{  //白灰
		TFT_DrawLine(x,y,x+w,y,0xEF7D);
		TFT_DrawLine(x+w,y,x+w,y+h,0x2965);
		TFT_DrawLine(x,y+h,x+w,y+h,0x2965);
		TFT_DrawLine(x,y,x,y+h,0xEF7D);
		}
	if (mode==2)	{  //红黄
		TFT_DrawLine(x,y,x+w,y,RED);
		TFT_DrawLine(x+w,y,x+w,y+h,YELLOW);
		TFT_DrawLine(x,y+h,x+w,y+h,YELLOW);
		TFT_DrawLine(x,y,x,y+h,RED);
	}
	if (mode==0)	{  //绿灰
		TFT_DrawLine(x,y,x+w,y,GREEN);
		TFT_DrawLine(x+w,y,x+w,y+h,PINK);
		TFT_DrawLine(x,y+h,x+w,y+h,PINK);
		TFT_DrawLine(x,y,x,y+h,GREEN);
	}
	if (mode==3)	{   //白灰2
		TFT_DrawLine(x,y,x+w,y,WHITE);
		TFT_DrawLine(x+w,y,x+w,y+h,GRAY0);
		TFT_DrawLine(x,y+h,x+w,y+h,GRAY0);
		TFT_DrawLine(x,y,x,y+h,WHITE);
	}
}

/**
 * @brief 显示图片
    * @param x 图片左上角 x 坐标
    * @param y 图片左上角 y 坐标
    * @param length 图片宽度
    * @param width 图片高度
    * @param p 图片数据指针，数据格式为 RGB565，每个像素占 2 字节

 */
void TFT_ShowImage(uint16_t x, uint16_t y, uint16_t length, uint16_t width, const unsigned char *p)
{
    TFT_SetRegion(x, y, x+length-1, y+width-1);
    for (int i = 0; i < length * width; i++)
    {
        uint8_t picL = p[2*i];
        uint8_t picH = p[2*i+1];
        TFT_Send16Bit((picH << 8) | picL);
    }
    TFT_SetRegion(0, 0, 127, 159);
}

/**
 * @brief 显示一个字符（8x16 点阵）
    * @param x 字符左上角 x 坐标
    * @param y 字符左上角 y 坐标
    * @param fc 字符颜色
    * @param bc 背景颜色
    * @param c 待显示字符（ASCII 码，范围 32-126）

 */
void TFT_ShowChar(uint8_t x, uint8_t y, uint16_t fc, uint16_t bc, char c)
{
    int k = (c - 32) * 16;
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 8; j++)
        {
            if (asc[k + i] & (0x80 >> j))
                TFT_DrawPoint(x + j, y + i, fc);
            else
                TFT_DrawPoint(x + j, y + i, bc);
        }
}

/**
 * @brief 显示字符串（自动换行）
    * @param x 起始列地址
    * @param y 起始行地址
    * @param fc 字符颜色
    * @param bc 背景颜色
    * @param c 待显示字符串（以 '\0' 结尾）
 */
void TFT_ShowString(uint8_t x, uint8_t y, uint16_t fc, uint16_t bc, char *c)
{
    int len = strlen(c);
    for (int i = 0; i < len; i++)
    {
        if (x >= 128) { x = 0; y += 16; }
        TFT_ShowChar(x, y, fc, bc, c[i]);
        x += 8;
    }
}

/**
 * @brief 显示整数（支持负数）
    * @param x 起始列地址
    * @param y 起始行地址
    * @param fc 字符颜色
    * @param bc 背景颜色
    * @param num 待显示整数

 */
void TFT_ShowNumber(uint8_t x, uint8_t y, uint16_t fc, uint16_t bc, long long num)
{
    char s[20];
    uint8_t k = 0;
    long long t = num;
    while (t) { t /= 10; k++; }
    if (num < 0)
    {
        s[0] = '-';
        s[k+1] = '\0';
        num = -num;
    }
    else
    {
        s[k] = '\0';
        if (k > 0) k--;
    }
    while (num)
    {
        s[k--] = '0' + (num % 10);
        num /= 10;
    }
    TFT_ShowString(x, y, fc, bc, s);
}

/**
 * @brief 映射中文字符串在字库中的索引
    * @param c 待映射的中文字符（UTF-8 编码，占 3 字节）
    * @retval 字符在字库中的索引，-1 表示未找到
 */
int map(char *c)
{
    int l1 = strlen(font_sample);
    for (int i = 0; i < l1; i += 3)
    {
        if (font_sample[i] == c[0] && font_sample[i+1] == c[1] && font_sample[i+2] == c[2])
            return i / 3;
    }
    return -1;
}

/**
 * @brief 显示中文字符串（混合显示，16x16 点阵）
    * @param x 起始列地址
    * @param y 起始行地址
    * @param fc 字符颜色
    * @param bc 背景颜色
    * @param c 待显示字符串（以 '\0' 结尾，支持 ASCII 和 UTF-8 编码的中文字符）

 */
void TFT_ShowChinese(uint8_t x, uint8_t y, uint16_t fc, uint16_t bc, char *c)
{
    int t = strlen(c);
    for (int n = 0; n < t; n++)
    {
        // 英文字符
        if (c[n] > 31 && c[n] < 127)
        {
            if (x + 8 >= 128) { x = 0; y += 16; }
            TFT_ShowChar(x, y, fc, bc, c[n]);
            x += 8;
            continue;
        }
        // 中文字符（UTF-8 编码占 3 字节）
        char tem[4] = {c[n], c[n+1], c[n+2], '\0'};
        int k = map(tem);
        if (k == -1)
        {
            if (x + 8 >= 128) { x = 0; y += 16; }
            TFT_ShowChar(x, y, YELLOW, RED, '?');
            x += 8;
            n += 2;
            continue;
        }
        if (x + 16 >= 128) { x = 0; y += 16; }
        for (int i = 0; i < 16; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                if (chinese_font[k*32 + 2*i] & (0x80 >> j))
                    TFT_DrawPoint(x + j, y + i, fc);
                else
                    TFT_DrawPoint(x + j, y + i, bc);
            }
            for (int j = 0; j < 8; j++)
            {
                if (chinese_font[k*32 + 2*i + 1] & (0x80 >> j))
                    TFT_DrawPoint(x + j + 8, y + i, fc);
                else
                    TFT_DrawPoint(x + j + 8, y + i, bc);
            }
        }
        x += 16;
        n += 2;
    }
}
