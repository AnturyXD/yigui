#include "bsp_OLED.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "font.h"



#define SCL_0    OLED_SCL_GPIO->BSRR = OLED_SCL_PIN << 16
#define SCL_1    OLED_SCL_GPIO->BSRR = OLED_SCL_PIN
#define SDA_0    OLED_SDA_GPIO->BSRR = OLED_SDA_PIN << 16
#define SDA_1    OLED_SDA_GPIO->BSRR = OLED_SDA_PIN



/*****************************************************************************
 ** 全局变量
****************************************************************************/
typedef struct                  // OLED 重要参数集
{
    uint16_t width;			    // OLED 宽度
    uint16_t height;		    // OLED 高度
    uint8_t  FlagInit;          // 初始化完成标志
} xOLED_TypeDef;
xOLED_TypeDef xLCD;	            // 管理LCD重要参数



uint8_t OLED_GRAM[128][8];



/******************************************************************************
 * 函  数： delay_ms
 * 功  能： ms 延时函数
 * 备  注： 1、系统时钟168MHz
 *          2、打勾：Options/ c++ / One ELF Section per Function
            3、编译优化级别：Level 3(-O3)
 * 参  数： uint32_t  ms  毫秒值
 * 返回值： 无
 ******************************************************************************/
static volatile uint32_t ulTimesMS;    // 使用volatile声明，防止变量被编译器优化
static void delay_ms(uint16_t ms)
{
    ulTimesMS = ms * 16500;
    while (ulTimesMS)
        ulTimesMS--;                   // 操作外部变量，防止空循环被编译器优化掉
}



//延时
static volatile uint32_t ulTimesUS;    // 使用volatile声明，防止变量被编译器优化
static void delay(void)
{
    ulTimesUS = 15;
    while (ulTimesUS--);
}



//起始信号
static void start(void)
{
    SDA_1;
    SCL_1;
    delay();
    SDA_0;
    delay();
    SCL_0;
    delay();
}

//结束信号
static void stop(void)
{
    SDA_0;
    SCL_1;
    delay();
    SDA_1;
}

//等待信号响应
static void waitACK(void)
{
    SDA_1;
    delay();
    SCL_1;
    delay();
    SCL_0;
    delay();
}

//写入一个字节
static void sendByte(uint8_t dat)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        if (dat & 0x80)
        {
            SDA_1;
        }
        else
        {
            SDA_0;
        }
        delay();
        SCL_1;
        delay();
        SCL_0;
        dat <<= 1;
    }
}

// 发送命令
static void writeCMD(uint8_t cmd)
{
    start();
    sendByte(0x78);  // 地址
    waitACK();
    sendByte(0x00);  // 命令：0x0
    waitACK();
    sendByte(cmd);   // 命令值
    waitACK();
    stop();
}

// 发送数据
//static void writeData(uint8_t data)
//{
//    start();
//    sendByte(0x78);  // 地址
//    waitACK();
//    sendByte(0x40);  // 命令：0x1
//    waitACK();
//    sendByte(data);  // 数据值
//    waitACK();
//    stop();
//}



//画点，x:0~127，y:0~63，t:1 填充 0,清空
static void drawPoint(uint8_t x, uint8_t y, uint8_t t)
{
    uint8_t i, m, n;
    i = y / 8;
    m = y % 8;
    n = 1 << m;
    if (t)
    {
        OLED_GRAM[x][i] |= n;
    }
    else
    {
        OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
        OLED_GRAM[x][i] |= n;
        OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
    }
}



/******************************************************************
 * 函数名： OLED_Init
 * 功  能： OLED的初始化,
 * 参  数： 无
 * 返  回:  无
 *****************************************************************/
void OLED_Init(void)
{
    GPIO_InitTypeDef    GPIO_InitStruct = {0};                  // 声明初始化要用到的结构体

    // 使能SCL引脚端口时钟
    if (OLED_SCL_GPIO == GPIOA)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN ;
    if (OLED_SCL_GPIO == GPIOB)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN ;
    if (OLED_SCL_GPIO == GPIOC)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN ;
    if (OLED_SCL_GPIO == GPIOD)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN ;
    if (OLED_SCL_GPIO == GPIOE)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN ;
    if (OLED_SCL_GPIO == GPIOF)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN ;
    if (OLED_SCL_GPIO == GPIOG)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOGEN ;
    // 使能SDA引脚端口时钟
    if (OLED_SDA_GPIO == GPIOA)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN ;
    if (OLED_SDA_GPIO == GPIOB)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN ;
    if (OLED_SDA_GPIO == GPIOC)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN ;
    if (OLED_SDA_GPIO == GPIOD)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN ;
    if (OLED_SDA_GPIO == GPIOE)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN ;
    if (OLED_SDA_GPIO == GPIOF)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN ;
    if (OLED_SDA_GPIO == GPIOG)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOGEN ;

#ifdef USE_STDPERIPH_DRIVER                                     // 标准库 配置
    // 初始化SCL引脚
    GPIO_InitStruct.GPIO_Pin = OLED_SCL_PIN;                    // 引脚编号
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;                  // 输出模式
    GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;                 // 输出方式：开漏
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;             // 引脚速率
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;                   // 打开上拉电阻
    GPIO_Init(OLED_SCL_GPIO, &GPIO_InitStruct);                 // 初始化
    // 初始化SDA引脚
    GPIO_InitStruct.GPIO_Pin = OLED_SDA_PIN;                    // 引脚编号
    GPIO_Init(OLED_SDA_GPIO, &GPIO_InitStruct);                 // 初始化
#endif

#ifdef USE_HAL_DRIVER                                           // HAL库 配置
    // 初始化SCL引脚
    GPIO_InitStruct.Pin   = OLED_SCL_PIN ;                      // 引脚编号
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;                // 工作模式
    GPIO_InitStruct.Pull  = GPIO_PULLUP;                        // 上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;          // 引脚速率
    HAL_GPIO_Init(OLED_SCL_GPIO, &GPIO_InitStruct);             // 初始化
    // 初始化SDA引脚
    GPIO_InitStruct.Pin   = OLED_SDA_PIN;                       // 引脚编号
    HAL_GPIO_Init(OLED_SDA_GPIO, &GPIO_InitStruct);             // 初始化
#endif

    delay_ms(100);

    writeCMD(0xAE); //--turn off oled panel
    writeCMD(0x00); //---set low column address
    writeCMD(0x10); //---set high column address
    writeCMD(0x40); //--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
    writeCMD(0x81); //--set contrast control register
    writeCMD(0xCF); // Set SEG Output Current Brightness
    writeCMD(0xA1); //--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
    writeCMD(0xC8); //Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
    writeCMD(0xA6); //--set normal display
    writeCMD(0xA8); //--set multiplex ratio(1 to 64)
    writeCMD(0x3f); //--1/64 duty
    writeCMD(0xD3); //-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
    writeCMD(0x00); //-not offset
    writeCMD(0xd5); //--set display clock divide ratio/oscillator frequency
    writeCMD(0x80); //--set divide ratio, Set Clock as 100 Frames/Sec
    writeCMD(0xD9); //--set pre-charge period
    writeCMD(0xF1); //Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    writeCMD(0xDA); //--set com pins hardware configuration
    writeCMD(0x12);
    writeCMD(0xDB); //--set vcomh
    writeCMD(0x40); //Set VCOM Deselect Level
    writeCMD(0x20); //-Set Page Addressing Mode (0x00/0x01/0x02)
    writeCMD(0x02); //
    writeCMD(0x8D); //--set Charge Pump enable/disable
    writeCMD(0x14); //--set(0x10) disable
    writeCMD(0xA4); // Disable Entire Display On (0xa4/0xa5)
    writeCMD(0xA6); // Disable Inverse Display On (0xa6/a7)
    OLED_Clear();
    writeCMD(0xAF);

    xLCD.FlagInit = 1;
    xLCD.width = 128;
    xLCD.height = 64;
}



/******************************************************************
 * 函数名： OLED_ColorTurn
 * 功  能： 反显函数
 * 参  数： uint8_t i   0_正常显示，1_反色显示
 * 返  回： 无
 *****************************************************************/
void OLED_ColorTurn(uint8_t i)
{
    if (i == 0)
    {
        writeCMD(0xA6);   // 正常显示
    }
    if (i == 1)
    {
        writeCMD(0xA7);   // 反色显示
    }
}



/******************************************************************
 * 函数名： OLED_DisplayTurn
 * 功  能： 屏幕旋转180度
 * 参  数： uint8_t i   0_正常显示，1_水平反转显示
 * 返  回： 无
 *****************************************************************/
void OLED_DisplayTurn(uint8_t i)
{
    if (i == 0)
    {
        writeCMD(0xC8); //正常显示
        writeCMD(0xA1);
    }
    if (i == 1)
    {
        writeCMD(0xC0); //反转显示
        writeCMD(0xA0);
    }
}


/******************************************************************
 * 函数名： OLED_Display
 * 功  能： 开启OLED显示
 * 参  数： uint8_t   sw   0_关闭显示，1_打开显示
 * 返  回： 无
 *****************************************************************/
void OLED_Display(uint8_t sw)
{
    writeCMD(0x8D);       // 电荷泵使能
    if (sw)
    {
        writeCMD(0x14);   // 开启电荷泵
        writeCMD(0xAF);   // 点亮屏幕
    }
    else
    {
        writeCMD(0x10);   // 关闭电荷泵
        writeCMD(0xAE);   // 关闭屏幕
    }
}



/******************************************************************
 * 函数名： OLED_Refresh
 * 功  能： 更新显存到OLED
 * 参  数： 无
 * 返  回： 无
 *****************************************************************/
void OLED_Refresh(void)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        writeCMD(0xb0 + i); // 设置行起始地址
        writeCMD(0x00);     // 设置低列起始地址
        writeCMD(0x10);     // 设置高列起始地址
        start();
        sendByte(0x78);
        waitACK();
        sendByte(0x40);
        waitACK();
        for (uint8_t n = 0; n < 128; n++)
        {
            sendByte(OLED_GRAM[n][i]);
            waitACK();
        }
        stop();
    }
}



/******************************************************************
 * 函数名： OLED_Clear
 * 功  能： 清屏函数
 * 参  数： 无
 * 返  回： 无
 *****************************************************************/
void OLED_Clear(void)
{
    memset(OLED_GRAM, 0, 8 * 128); // 清除缓存所有数据
    OLED_Refresh();                // 更新显示
}



/******************************************************************
 * 函数名： OLED_Line
 * 功  能： 画直线
 * 参  数： uint16_t x1     起点X坐标
 *          uint16_t y1     起点Y坐标
 *          uint16_t x2     终点X坐标
 *          uint16_t y2     终点Y坐标
 *          uint16_t color  颜色值
 * 备  注：
 *****************************************************************/
void OLED_Line(uint16_t  x1, uint16_t  y1, uint16_t  x2, uint16_t  y2)
{
    uint16_t  t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1;                        // 计算坐标增量
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;
    if (delta_x > 0)incx = 1;                 // 设置单步方向
    else if (delta_x == 0)incx = 0;           // 垂直线
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0)incy = 1;
    else if (delta_y == 0)incy = 0;           // 水平线
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }
    if (delta_x > delta_y)distance = delta_x; // 选取基本增量坐标轴
    else distance = delta_y;
    for (t = 0; t <= distance + 1; t++)       // 画线输出
    {
        drawPoint(uRow, uCol, 1);             // 画点
        xerr += delta_x ;
        yerr += delta_y ;
        if (xerr > distance)
        {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            uCol += incy;
        }
    }
    OLED_Refresh();  // 更新显存到OLED
}



/******************************************************************
 * 函数名： OLED_Circle
 * 功  能： 在指定位置画圆
 * 参  数： uint16_t Xpos     X坐标
 *          uint16_t Ypos     起点Y坐标
 *          uint16_t Radius   半径
 * 备  注：
 *****************************************************************/
void OLED_Circle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius)
{
    int16_t mx = Xpos, my = Ypos, x = 0, y = Radius;
    int16_t d = 1 - Radius;
    while (y > x)
    {
        drawPoint(x + mx, y + my, 1);
        drawPoint(-x + mx, y + my, 1);
        drawPoint(-x + mx, -y + my, 1);
        drawPoint(x + mx, -y + my, 1);
        drawPoint(y + mx, x + my, 1);
        drawPoint(-y + mx, x + my, 1);
        drawPoint(y + mx, -x + my, 1);
        drawPoint(-y + mx, -x + my, 1);
        if (d < 0)
        {
            d += 2 * x + 3;
        }
        else
        {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
    OLED_Refresh();  // 更新显存到OLED
}



/******************************************************************
 * 函数名： OLED_ShowChinese
 * 功  能： 显示自行取模的汉字,
 *          字库数据在font文件中，只适合少量汉字固定输出
 *          PCtoLCD2018取模：阴码+列行式+逆向+C51格式
 * 参  数： uint16_t  x         坐标x
 *          uint16_t  y         坐标y
 *          uint8_t   index     字模数据在数组中的序号
 * 返  回:  无
 *****************************************************************/
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t num, uint8_t size1)
{
    uint8_t m, temp;
    uint8_t x0 = x, y0 = y;
    uint16_t size3 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * size1; // 得到字体一个字符对应点阵集所占的字节数

    for (uint16_t i = 0; i < size3; i++)
    {
        if (size1 == 12)
        {
            temp = aFontChinese12[num][i];   // 调用12*12字体
        }
        else if (size1 == 16)
        {
            temp = aFontChinese16[num][i];   // 调用16*16字体
        }
        else if (size1 == 24)
        {
            temp = aFontChinese24[num][i];   // 调用24*24字体
        }
        else if (size1 == 32)
        {
            temp = aFontChinese32[num][i];   // 调用32*32字体
        }
        else
        {
            temp = aFontChinese12[num][i];   // 如果是非法字形，则调用12*12字体
        }
        for (m = 0; m < 8; m++)
        {
            if (temp & 0x01)
                drawPoint(x, y, 1);
            else
                drawPoint(x, y, 0);
            temp >>= 1;
            y++;
        }
        x++;
        if ((x - x0) == size1)
        {
            x = x0;
            y0 = y0 + 8;
        }
        y = y0;
    }
    OLED_Refresh();                          // 更新显存到OLED
}



/******************************************************************
 * 函数名： drawAscii
 * 功  能： 在指定位置显示一个字符
 * 参  数： uint16_t x,y     起始坐标
 *          uint8_t  num     要显示的字符:" "--->"~"
 *          uint8_t  size    字体大小 12/16/24/32
 *****************************************************************/
static void drawAscii(uint16_t x, uint16_t y, uint8_t num, uint8_t size)
{
    static uint8_t temp;
    static uint8_t csize;
    static uint16_t y0;

    y0 = y;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);   // 得到字体一个字符对应点阵集所占的字节数
    num = num - ' ';                                          // 得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
    for (uint8_t t = 0; t < csize; t++)
    {
        if (size == 12)         temp = aFontASCII12[num][t];  // 调用1206字体
        else if (size == 16)    temp = aFontASCII16[num][t];  // 调用1608字体
        else if (size == 24)    temp = aFontASCII24[num][t];  // 调用2412字体
        else if (size == 32)    temp = aFontASCII32[num][t];  // 调用3216字体
        else return;                                          // 没有的字库

        for (uint8_t t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80) drawPoint(x, y,  1);             // 字体 画点
            else             drawPoint(x, y,  0);             // 背景 画点

            temp <<= 1;
            y++;
            if (y >= xLCD.height)    return;                  // 超出屏幕高度(底)
            if ((y - y0) == size)
            {
                y = y0;
                x++;
                if (x >= xLCD.width) return;                  // 超出屏幕宽度(宽)
                break;
            }
        }
    }
}



/******************************************************************
 * 函数名： drawGBK
 * 功  能： 在指定位置显示一个字符
 * 参  数： uint16_t x,y     起始坐标
 *          uint8_t  num     要显示的字符:" "--->"~"
 *          uint8_t  size    字体大小 12/16/24/32
 *****************************************************************/
static void drawGBK(uint16_t x, uint16_t y, uint8_t *font, uint8_t size)
{
    static uint8_t temp;
    static uint16_t y0;
    static uint8_t GBK[128];
    static uint8_t csize;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);   // 得到字体一个字符对应点阵集所占的字节数
    W25Q128_ReadFontData(font, size, GBK);                // 得到相应大小的点阵数据

    y0 = y;
    for (uint8_t t = 0; t < csize; t++)
    {
        temp = GBK[t];                                    // 得到GBK点阵数据
        for (uint8_t t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)   drawPoint(x, y, 1);
            else               drawPoint(x, y, 0);
            temp <<= 1;
            y++;
            if ((y - y0) == size)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}



/******************************************************************************
 * 函  数： OLED_String
 * 功  能： 在LCD上显示字符串(支持英文、汉字)
 * 描  述： 英文：字模数据保存在font.h，编译后和代码一起保存在芯片内部Flash
 *          汉字：字模保存在外部Flash中，本函数字库在W25Q128中
 *                魔女开发板中W25Q128已烧录宋体4种字号大小字模数据
 * 参  数： uint16_t   x      字体左上角X坐标
 *          uint16_t   y      字体左上角y坐标
 *          char*      pFont  要显示的字符串数据
 *          uint8_t    size   字号大小：12 16 24 32
 * 返回值:  无
 ******************************************************************************/
void OLED_String(uint16_t x, uint16_t y, char *pFont, uint8_t size)
{
    if (xLCD .FlagInit == 0) return;

    uint16_t xStart = x;

    if (size != 12 && size != 16 && size != 24 && size != 32) // 字体大小控制
        size = 24;

    while (*pFont != 0)                            // 连续读取字符串数据，直到'\0'时停止
    {
        if (x > (xLCD.width - size))               // 行位置判断，如果到了行末，就把光标换行
        {
            x = xStart;
            y = y + size;
        }
        if (y > (xLCD.height - size))              // 列位置判断，如果到了列末，就返回，不再输出
            return;

        if (*pFont < 127)                          // ASCII字符
        {
            drawAscii(x, y, *pFont, size);
            pFont++;
            x += size / 2;
        }
        else                                       // 汉字显示
        {
            // 重要: 如果用的不是魔女开发板的字库, 就要修改或注释下面这一行, 这样就不影响ASCII英文字符的输出
            drawGBK(x, y, (uint8_t *)pFont, size);
            pFont = pFont + 2;                     // 下一个要显示的数据在内存中的位置
            x = x + size;                          // 下一个要显示的数据在屏幕上的X位置
        }
    }
    OLED_Refresh();                                // 更新显存到OLED
}








