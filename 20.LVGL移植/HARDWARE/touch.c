#include "touch.h"
#include "lcd.h"
#include "stdlib.h"
#include "math.h"
#include "stdio.h"
#include "usart.h"
#include "delay_us.h"
#include "gpio.h"

_m_tp_dev tp_dev =
{
    TP_Init,
    TP_Scan,
    TP_Adjust,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

#if (DFT_SCAN_DIR>>4)%4
uint8_t CMD_RDX=0X90;
uint8_t CMD_RDY=0XD0;
#else
uint8_t CMD_RDX=0XD0;
uint8_t CMD_RDY=0X90;
#endif

// SPI写数据
// 向触摸IC写入1字节数据
// num: 要写入的数据
void TP_Write_Byte(uint8_t num)
{
    uint8_t count=0;
    for(count=0;count<8;count++)
    {
        if(num&0x80)
        {
            HAL_GPIO_WritePin(TDIN_GPIO_Port, TDIN_Pin,GPIO_PIN_SET);
        }
        else
        {
            HAL_GPIO_WritePin(TDIN_GPIO_Port, TDIN_Pin,GPIO_PIN_RESET);
        }
        num<<=1;
        HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_RESET);
        delay_us(1);
        HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_SET);
    }
}

// SPI读数据
// 从触摸IC读取adc值
// CMD: 命令
// 返回值: 读到的数据
uint16_t TP_Read_AD(uint8_t CMD)
{
    uint8_t count=0;
    uint16_t Num=0;
    HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TDIN_GPIO_Port, TDIN_Pin,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TCS_GPIO_Port, TCS_Pin,GPIO_PIN_RESET);
    TP_Write_Byte(CMD);
    delay_us(6);
    HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_RESET);
    delay_us(1);
    HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_SET);
    delay_us(1);
    HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_RESET);
    for(count=0;count<16;count++)
    {
        Num<<=1;
        HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_RESET);
        delay_us(1);
        HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_SET);
        if(HAL_GPIO_ReadPin(DOUT_GPIO_Port, DOUT_Pin)==GPIO_PIN_SET)
        {
            Num++;
        }
    }
    Num>>=4;
    HAL_GPIO_WritePin(TCS_GPIO_Port, TCS_Pin,GPIO_PIN_SET);
    printf("CMD: 0x%02X, Read: %d\r\n", CMD, Num);
    return(Num);
}

// 读取一个坐标值(x或者y)
// 连续读取READ_TIMES次数据,去掉最大值和最小值,取平均值
// xy: 命令(CMD_RDX/CMD_RDY)
// 返回值: 读到的数据
#define READ_TIMES 5
#define LOST_VAL 1
uint16_t TP_Read_XOY(uint8_t xy)
{
    uint16_t i, j;
    uint16_t buf[READ_TIMES];
    uint16_t sum=0;
    uint16_t temp;
    for(i=0;i<READ_TIMES;i++)buf[i]=TP_Read_AD(xy);
    for(i=0;i<READ_TIMES-1; i++)
    {
        for(j=i+1;j<READ_TIMES;j++)
        {
            if(buf[i]>buf[j])
            {
                temp=buf[i];
                buf[i]=buf[j];
                buf[j]=temp;
            }
        }
    }
    sum=0;
    for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];
    temp=sum/(READ_TIMES-2*LOST_VAL);
    return temp;
}

// 读取x,y坐标
// 误差小于100
// x,y: 读到的坐标值
// 返回值: 0,失败;1,成功
uint8_t TP_Read_XY(uint16_t *x,uint16_t *y)
{
    uint16_t xtemp,ytemp;
    xtemp=TP_Read_XOY(CMD_RDX);
    ytemp=TP_Read_XOY(CMD_RDY);
    *x=xtemp;
    *y=ytemp;
    return 1;
}

// 连续2次读取触摸IC,两次误差在ERR_RANGE内认为有效
// ERR_RANGE:误差范围
// x,y: 读到的坐标值
// 返回值: 0,失败;1,成功
#define ERR_RANGE 50
uint8_t TP_Read_XY2(uint16_t *x,uint16_t *y)
{
    uint16_t x1,y1;
    uint16_t x2,y2;
    uint8_t flag;
    flag=TP_Read_XY(&x1,&y1);
    if(flag==0)return(0);
    flag=TP_Read_XY(&x2,&y2);
    if(flag==0)return(0);
    if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))
    &&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
        return 1;
    }else return 0;
}

// 画触摸屏校准点
// x,y:坐标
// color:颜色
void TP_Drow_Touch_Point(uint16_t x,uint16_t y,uint16_t color)
{
    LCD_DrawLine(x-12,y,x+13,y,color);
    LCD_DrawLine(x,y-12,x,y+13,color);
    LCD_DrawPoint(x+1,y+1,color);
    LCD_DrawPoint(x-1,y+1,color);
    LCD_DrawPoint(x+1,y-1,color);
    LCD_DrawPoint(x-1,y-1,color);
    Draw_Circle(x,y,6,color);
}

// 画一个大点(2*2)
// x,y:坐标
// color:颜色
void TP_Draw_Big_Point(uint16_t x,uint16_t y,uint16_t color)
{
    LCD_DrawPoint(x,y,color);
    LCD_DrawPoint(x+1,y,color);
    LCD_DrawPoint(x,y+1,color);
    LCD_DrawPoint(x+1,y+1,color);
}

/******************************************************************************
      画线函数
      参数: x1,y1   起始坐标
                x2,y2   结束坐标
                color   线颜色
      返回值: 无
******************************************************************************/
void LCD_DrawRoughLine(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color)
{
    uint16_t t;
    int xerr=0,yerr=0,delta_x,delta_y,distance;
    int incx,incy,uRow,uCol;
    delta_x=x2-x1;
    delta_y=y2-y1;
    uRow=x1;
    uCol=y1;
    if(delta_x>0)incx=1;
    else if (delta_x==0)incx=0;
    else {incx=-1;delta_x=-delta_x;}
    if(delta_y>0)incy=1;
    else if (delta_y==0)incy=0;
    else {incy=-1;delta_y=-delta_y;}
    if(delta_x>delta_y)distance=delta_x;
    else distance=delta_y;
    for(t=0;t<distance+1;t++)
    {
        TP_Draw_Big_Point(uRow,uCol,color);
        xerr+=delta_x;
        yerr+=delta_y;
        if(xerr>distance)
        {
            xerr-=distance;
            uRow+=incx;
        }
        if(yerr>distance)
        {
            yerr-=distance;
            uCol+=incy;
        }
    }
}

// 触摸屏扫描
// tp:0,屏幕坐标;1,原始坐标(校准时使用)
// 返回值: 当前触摸状态
// 0,无触摸;1,有触摸
uint8_t TP_Scan(uint8_t tp)
{
    if(HAL_GPIO_ReadPin(PEN_GPIO_Port, PEN_Pin)==RESET)
    {
        if(tp)TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]);
        else if(TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]))
        {
            tp_dev.x[0]=tp_dev.xfac*tp_dev.x[0]+tp_dev.xoff;
            tp_dev.y[0]=tp_dev.yfac*tp_dev.y[0]+tp_dev.yoff;
        }
        if((tp_dev.sta&TP_PRES_DOWN)==0)
        {
            tp_dev.sta=TP_PRES_DOWN|TP_CATH_PRES;
            tp_dev.x[4]=tp_dev.x[0];
            tp_dev.y[4]=tp_dev.y[0];
        }
    }else
    {
        if(tp_dev.sta&TP_PRES_DOWN)
        {
            tp_dev.sta&=~(1<<7);
        }else
        {
            tp_dev.x[4]=0;
            tp_dev.y[4]=0;
            tp_dev.x[0]=0xffff;
            tp_dev.y[0]=0xffff;
        }
    }
    return tp_dev.sta&TP_PRES_DOWN;
}

// 保存校准数据到EEPROM
#define SAVE_ADDR_BASE 40
void TP_Save_Adjdata(void)
{
}

// 从EEPROM读取校准数据
// 返回值:1,成功读取
//        0,读取失败,需要重新校准
uint8_t TP_Get_Adjdata(void)
{
    return 0;
}

// 触摸屏校准程序
void TP_Adjust(void)
{
    uint16_t pos_temp[4][2];
    uint8_t  cnt=0;
    uint16_t d1,d2;
    uint32_t tem1,tem2;
    double fac;
    cnt=0;
    LCD_Fill(0,0,lcddev.width,lcddev.height,WHITE);
    LCD_ShowString(5,40,"Please adjustment the screen.",RED,WHITE,16,0);
    TP_Drow_Touch_Point(20,20,RED);
    tp_dev.sta=0;
    tp_dev.xfac=0;
    while(1)
    {
        tp_dev.scan(1);
        if((tp_dev.sta&0xc0)==TP_CATH_PRES)
        {
            tp_dev.sta&=~(1<<6);

            pos_temp[cnt][0]=tp_dev.x[0];
            pos_temp[cnt][1]=tp_dev.y[0];
            cnt++;
            switch(cnt)
            {
                case 1:
                    TP_Drow_Touch_Point(20,20,WHITE);
                    TP_Drow_Touch_Point(lcddev.width-20,20,RED);
                    break;
                case 2:
                    TP_Drow_Touch_Point(lcddev.width-20,20,WHITE);
                    TP_Drow_Touch_Point(20,lcddev.height-20,RED);
                    break;
                case 3:
                    TP_Drow_Touch_Point(20,lcddev.height-20,WHITE);
                    TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,RED);
                    break;
                case 4:
                    printf("Point1: X=%d, Y=%d\n", pos_temp[0][0], pos_temp[0][1]);
                    printf("Point2: X=%d, Y=%d\n", pos_temp[1][0], pos_temp[1][1]);
                    printf("Point3: X=%d, Y=%d\n", pos_temp[2][0], pos_temp[2][1]);
                    printf("Point4: X=%d, Y=%d\n", pos_temp[3][0], pos_temp[3][1]);

                    tem1=abs(pos_temp[0][0]-pos_temp[1][0]);
                    tem2=abs(pos_temp[0][1]-pos_temp[1][1]);
                    tem1*=tem1;
                    tem2*=tem2;
                    d1=sqrt(tem1+tem2);

                    tem1=abs(pos_temp[2][0]-pos_temp[3][0]);
                    tem2=abs(pos_temp[2][1]-pos_temp[3][1]);
                    tem1*=tem1;
                    tem2*=tem2;
                    d2=sqrt(tem1+tem2);
                    fac=(float)d1/d2;
                    if(fac<0.95||fac>1.05||d1==0||d2==0)
                    {
                        cnt=0;
                        TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);
                        TP_Drow_Touch_Point(20,20,RED);
                        LCD_ShowString(5,40,"Touch Adjust Filed!          ",RED,WHITE,16,0);
                        LCD_ShowString(5,60,"Please Adjust Again!         ",RED,WHITE,16,0);
                        HAL_Delay(1000);
                        continue;
                    }
                    tem1=abs(pos_temp[0][0]-pos_temp[2][0]);
                    tem2=abs(pos_temp[0][1]-pos_temp[2][1]);
                    tem1*=tem1;
                    tem2*=tem2;
                    d1=sqrt(tem1+tem2);

                    tem1=abs(pos_temp[1][0]-pos_temp[3][0]);
                    tem2=abs(pos_temp[1][1]-pos_temp[3][1]);
                    tem1*=tem1;
                    tem2*=tem2;
                    d2=sqrt(tem1+tem2);
                    fac=(float)d1/d2;
                    if(fac<0.95||fac>1.05)
                    {
                        cnt=0;
                        TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);
                        TP_Drow_Touch_Point(20,20,RED);
                        LCD_ShowString(5,40,"Touch Adjust Filed!          ",RED,WHITE,16,0);
                        LCD_ShowString(5,60,"Please Adjust Again!         ",RED,WHITE,16,0);
                        HAL_Delay(1000);
                        continue;
                    }

                    tem1=abs(pos_temp[1][0]-pos_temp[2][0]);
                    tem2=abs(pos_temp[1][1]-pos_temp[2][1]);
                    tem1*=tem1;
                    tem2*=tem2;
                    d1=sqrt(tem1+tem2);

                    tem1=abs(pos_temp[0][0]-pos_temp[3][0]);
                    tem2=abs(pos_temp[0][1]-pos_temp[3][1]);
                    tem1*=tem1;
                    tem2*=tem2;
                    d2=sqrt(tem1+tem2);
                    fac=(float)d1/d2;
                    if(fac<0.95||fac>1.05)
                    {
                        cnt=0;
                        TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);
                        TP_Drow_Touch_Point(20,20,RED);
                        LCD_ShowString(5,40,"Touch Adjust Filed!          ",RED,WHITE,16,0);
                        LCD_ShowString(5,60,"Please Adjust Again!         ",RED,WHITE,16,0);
                        HAL_Delay(1000);
                        continue;
                    }
                    tp_dev.xfac=(float)(lcddev.width-40)/(pos_temp[1][0]-pos_temp[0][0]);
                    tp_dev.xoff=(lcddev.width-tp_dev.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;

                    tp_dev.yfac=(float)(lcddev.height-40)/(pos_temp[2][1]-pos_temp[0][1]);
                    tp_dev.yoff=(lcddev.height-tp_dev.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;

                    TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);
                    LCD_ShowString(5,40,"Touch Screen Adjust OK!      ",RED,WHITE,16,0);
                    LCD_ShowString(5,60,"                             ",RED,WHITE,16,0);
                    HAL_Delay(1000);
                    LCD_ShowString(5,40,"                       ",RED,WHITE,16,0);
                    TP_Save_Adjdata();
                    return;
            }
        }
    }
}

// 触摸屏初始化
uint8_t TP_Init(void)
{
    TP_Read_XY(&tp_dev.x[0],&tp_dev.y[0]);
    TP_Adjust();
    TP_Get_Adjdata();
    return 1;
}

void Touch_Test(void)
{
    uint16_t lastpos[2];
    lastpos[0]=0xffff;
    TP_Init();
    LCD_Clear(WHITE);
    while(1)
    {
        tp_dev.scan(0);
        if(tp_dev.sta&TP_PRES_DOWN)
        {
            HAL_Delay(1);
            if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
            {
                if(lastpos[0]==0XFFFF)
                {
                    lastpos[0]=tp_dev.x[0];
                    lastpos[1]=tp_dev.y[0];
                }
                LCD_DrawRoughLine(lastpos[0],lastpos[1],tp_dev.x[0],tp_dev.y[0],RED);
                lastpos[0]=tp_dev.x[0];
                lastpos[1]=tp_dev.y[0];
                LCD_ShowString(10,lcddev.height-60,"X:",RED,BLACK,16,0);
                LCD_ShowIntNum(26,lcddev.height-60,tp_dev.x[0],3,RED,BLACK,16);
                LCD_ShowString(10,lcddev.height-40,"Y:",RED,BLACK,16,0);
                LCD_ShowIntNum(26,lcddev.height-40,tp_dev.y[0],3,RED,BLACK,16);
            }
        }
    }
}


