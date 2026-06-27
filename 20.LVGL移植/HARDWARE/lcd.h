#ifndef __LCD_H
#define __LCD_H
#include "main.h"
#include "stdlib.h"

typedef __IO uint32_t  vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;

#define DFT_SCAN_DIR  U2D_R2L  //默认扫描方向

//扫描方向定义
#define L2R_U2D  0x00 //从左到右,从上到下
#define L2R_D2U  0x80 //从左到右,从下到上
#define R2L_U2D  0x40 //从右到左,从上到下
#define R2L_D2U  0xc0 //从右到左,从下到上

#define U2D_L2R  0x20 //从上到下,从左到右
#define U2D_R2L  0x60 //从上到下,从右到左
#define D2U_L2R  0xa0 //从下到上,从左到右
#define D2U_R2L  0xe0 //从下到上,从右到左

//LCD重要参数集
typedef struct
{
	uint16_t width;			//LCD宽度
	uint16_t height;		//LCD高度
	uint16_t id;			//LCD ID
}_lcd_dev;

//LCD参数
extern _lcd_dev lcddev;	//管理LCD重要参数

//////////////////////////////////////////////////////////////////////////////////
//-----------------LCD端口定义----------------
//LCD地址结构体
typedef struct
{
	vu16 LCD_REG;
	vu16 LCD_RAM;
} LCD_TypeDef;

//使用NOR/SRAM的 Bank1.sector4,地址位HADDR[27,26]=11 A10作为数据/命令控制脚
//注意：STM32内部地址是按字节寻址的！
#define LCD_BASE        ((uint32_t)(0x6C000000 | 0x0000007E))
#define LCD             ((LCD_TypeDef *) LCD_BASE)
//////////////////////////////////////////////////////////////////////////////////

//常用颜色
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000
#define BLUE         	 0x001F
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40 //棕色
#define BRRED 			 0XFC07 //棕红色
#define GRAY  			 0X8430 //灰色

//GUI颜色
#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色
#define GRAYBLUE       	 0X5458 //灰蓝色

#define LIGHTGREEN     	 0X841F //浅绿色
#define LGRAY 			 0XC618 //浅灰色

#define LGRAYBLUE        0XA651 //浅灰蓝色
#define LBBLUE           0X2B12 //浅棕蓝色

void LCD_WR_REG(uint16_t reg);			//写寄存器
void LCD_WR_DATA(uint16_t data);		//写数据
uint16_t LCD_RD_DATA(void);				//读数据
uint16_t LCD_ReadReg(uint16_t LCD_Reg);	//读寄存器
void LCD_SetCursor(uint16_t x,uint16_t y);	//设置光标位置
void LCD_Address_Set(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2);	//设置显示区域
void LCD_DisplayOn(void);				//开启显示
void LCD_DisplayOff(void);				//关闭显示
uint16_t LCD_ReadPoint(uint16_t x,uint16_t y);	//读点
void LCD_Clear(uint16_t color);			//清屏

void LCD_Fill(uint16_t xsta,uint16_t ysta,uint16_t xend,uint16_t yend,uint16_t color);	//指定区域填充颜色
void LCD_DrawPoint(uint16_t x,uint16_t y,uint16_t color);	//在指定位置画一个点
void LCD_DrawLine(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color);	//画一条线
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t color);	//画矩形
void Draw_Circle(uint16_t x0,uint16_t y0,uint8_t r,uint16_t color);	//画圆

void LCD_ShowChinese(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);	//显示汉字串
void LCD_ShowChinese12x12(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);	//显示12x12汉字
void LCD_ShowChinese16x16(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);	//显示16x16汉字
void LCD_ShowChinese24x24(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);	//显示24x24汉字
void LCD_ShowChinese32x32(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);	//显示32x32汉字

void LCD_ShowChar(uint16_t x,uint16_t y,uint8_t num,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);	//显示一个字符
void LCD_ShowString(uint16_t x,uint16_t y,const uint8_t *p,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);	//显示字符串
uint32_t mypow(uint8_t m,uint8_t n);	//幂函数
void LCD_ShowIntNum(uint16_t x,uint16_t y,uint16_t num,uint8_t len,uint16_t fc,uint16_t bc,uint8_t sizey);	//显示整数
void LCD_ShowFloatNum1(uint16_t x,uint16_t y,float num,uint8_t len,uint16_t fc,uint16_t bc,uint8_t sizey);	//显示浮点数

void LCD_ShowPicture(uint16_t x,uint16_t y,uint16_t length,uint16_t width,const uint8_t pic[]);	//显示图片

void Set_Dir(uint8_t dir);	//设置显示方向
void LCD_Init(void);			//初始化

#endif



