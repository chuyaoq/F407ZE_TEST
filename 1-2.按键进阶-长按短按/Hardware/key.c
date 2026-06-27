#include "key.h"
#define     Key1         HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)  //key1--PA0
#define     Key2         HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_3)  //key2--PA3

#define     KEY_PORT    (Key1|Key2<<1|0xfc)

// uint8_t key_time=0;	// 长按计时
uint8_t Trg=0;			// 短暂记录按下情况
uint8_t Cont=0;			// 短暂记录按钮按着情况

void Key_Read(void)
{
    uint8_t ReadData = KEY_PORT^0xff;		// 将按下按钮的所在位置1
    Trg = ReadData & (ReadData ^ Cont);		// 记录按下情况
    Cont = ReadData;						// 记录按钮按着情况
}
