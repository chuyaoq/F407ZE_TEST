#include "key.h"
#define     Key1         HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)  //key1--PA0
#define     Key2         HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_3)  //key2--PA3

uint8_t Key_Read(void)
{   
	  static uint8_t keynum;                       //输出值
    if(Key1==GPIO_PIN_RESET&&keynum==0)            //判断按键1是否按下,并且之前已经抬起，防止多次判断，重复进入
		{
		   HAL_Delay(20);                              //消抖
			 if(Key1==GPIO_PIN_RESET)         //再次判断
			 {
             keynum = 1;			
             return keynum;				 
			 }
		}
		else if(Key2==GPIO_PIN_RESET&&keynum==0)        //判断按键2是否按下,并且之前已经抬起，防止多次判断，重复进入
		{
		   HAL_Delay(20);                               //消抖
			 if(Key2==GPIO_PIN_RESET)          //再次判断
			 {
					   keynum = 2;	
				     return keynum;	
			 }
		}
		else if(Key1!=GPIO_PIN_RESET&&Key2!=GPIO_PIN_RESET)   //如果key1和key2都未按下，则判断按键抬起。
		{
		     keynum = 0;
			   return keynum;	
		}
		
		return 0;
}

