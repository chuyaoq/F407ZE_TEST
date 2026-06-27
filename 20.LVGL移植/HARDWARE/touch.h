#ifndef __TOUCH_H__
#define __TOUCH_H__
#include "main.h"
#include "lcd.h"

#define TP_PRES_DOWN 0x80      // 触摸按下
#define TP_CATH_PRES 0x40      // 有触摸按下
#define CT_MAX_TOUCH  5        // 支持最多5个触摸点

// 触摸屏结构体
typedef struct
{
    uint8_t (*init)(void);          // 初始化触摸屏
    uint8_t (*scan)(uint8_t);       // 扫描触摸 0=屏幕坐标 1=原始坐标
    void (*adjust)(void);           // 触摸屏校准
    uint16_t x[CT_MAX_TOUCH];       // 当前坐标
    uint16_t y[CT_MAX_TOUCH];       // 最多支持5点触摸
    uint8_t  sta;                   // 状态
                                    // b7: 按下1/松开0
                                    // b6: 0=无触摸 1=有触摸
                                    // b5: 保留

    // 触摸屏校准参数
    float xfac;
    float yfac;
    short xoff;
    short yoff;

    uint8_t touchtype;
}_m_tp_dev;

extern _m_tp_dev tp_dev;    // 触摸屏结构体变量

// 触摸函数声明
void TP_Write_Byte(uint8_t num);
uint16_t TP_Read_AD(uint8_t CMD);
uint16_t TP_Read_XOY(uint8_t xy);
uint8_t TP_Read_XY(uint16_t *x,uint16_t *y);
uint8_t TP_Read_XY2(uint16_t *x,uint16_t *y);
void TP_Drow_Touch_Point(uint16_t x,uint16_t y,uint16_t color);
void TP_Draw_Big_Point(uint16_t x,uint16_t y,uint16_t color);
void LCD_DrawRoughLine(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color);

void TP_Adjust(void);
uint8_t TP_Scan(uint8_t tp);
uint8_t TP_Init(void);
void Touch_Test(void);

#endif


