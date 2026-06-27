#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BLE.h"
#include "usart.h"
#include "main.h"


uint8_t  BLE_Rxbuff[128];

//AT指令检验
uint8_t Find_Str(uint8_t* Str1, uint8_t* Str2)
{
	uint8_t p1=0, p2, temp;
	for(; *Str1 != '\0'; Str1++,p1++){
		temp = p2 = 0;
		while(*(Str2+p2) && *(Str1+temp) && *(Str2+p2) == *(Str1+temp)){
			p2++;
			temp++;
		}
		if(*(Str2+p2) == '\0'){							
			return p1+1;
		}
		if(*(Str1+temp) == '\0'){			
			return 0;
		}
	}
	return 0;
} 

//发送函数
void Uart3_send(uint8_t *ch)
{
	while(*ch!='\0'){
			HAL_UART_Transmit(&huart3,(uint8_t *)ch++, 1, 500); 
	}
}



uint8_t ESP_SendAT(uint8_t *ch,uint8_t *Restr,uint16_t timeout)
{
	
		uint16_t i=0;
		memset(BLE_Rxbuff,0,sizeof(BLE_Rxbuff));  //清空数据
		Uart3_send(ch); //发送
		while(i<timeout&&Find_Str(BLE_Rxbuff, Restr)==0)  //未超时并且未收到数据
		{
			    i++;
					HAL_Delay(1);
		}
		if(i<timeout)
		{
			    return 1;
		}
		return 0;
		
}



uint8_t  BLE_Set(void)  //初始化设置
{
	uint8_t i;
  for(i=0;i<20;i++)   //AT指令检测
	{
	    if(ESP_SendAT("AT\r\n","OK",1000))
        break;
	}
		if(ESP_SendAT("ATE0\r\n","OK",2000)) //取消回显
		  if(ESP_SendAT("AT+BLEINIT=2\r\n","OK",1000))//设置为蓝牙server模式
			  if(ESP_SendAT("AT+BLEGATTSSRVCRE\r\n","OK",1000)) //BLE服务端创建服务
			    if(ESP_SendAT("AT+BLEGATTSSRVSTART\r\n","OK",1000)) //BLE服务端开始服务
						if(ESP_SendAT("AT+BLEADVPARAM=50,50,0,0,7,0,,\r\n","OK",1000)) //设置广播服务
							if(ESP_SendAT("AT+BLEADVDATA=\"0201060A09457370726573736966030302A0\"\r\n","OK",1000)) //设置广播数据
									if(ESP_SendAT("AT+BLESPPCFG=1,1,7,1,5\r\n","OK",1000)) //配置SPP
										if(ESP_SendAT("AT+BLEADVSTART\r\n","OK",1000)) //开始广播
	                   return 1;
	return 0;
}





