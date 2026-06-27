#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ESP.h"
#include "usart.h"
#include "main.h"
uint8_t  Wifi_Rxbuff[256];

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
		memset(Wifi_Rxbuff,0,sizeof(Wifi_Rxbuff));  //清空数据
		Uart3_send(ch); //发送
		while(i<timeout&&Find_Str(Wifi_Rxbuff, Restr)==0)  //未超时并且未收到数据
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



uint8_t  WIFI_Set(void)  //WIFI初始化设置
{
	uint8_t i;
  for(i=0;i<20;i++)   //AT指令检测
	{
	    if(ESP_SendAT("AT\r\n","OK",1000))
      break;
	}
	if(ESP_SendAT("AT\r\n","OK",1000))//再次判断
	  if(ESP_SendAT("AT+CWMODE=1\r\n","OK",1000))//sta模式
        if(ESP_SendAT("ATE0\r\n","OK",2000)) //取消回显
			if(ESP_SendAT("AT+CWAUTOCONN=0\r\n","OK",2000)) //关闭自动连接
				if(ESP_SendAT("AT+CWJAP=\"chuchuchu\",\"11111111\"\r\n","OK",7000)) //连接WiFi
				  if(ESP_SendAT("AT+MQTTUSERCFG=0,1,\"esp32\",\"user4\",\"user4\",0,0,\"\"\r\n","OK",2000)) //设置MQTT连接参数
					  if(ESP_SendAT("AT+MQTTCONN=0,\"47.93.251.30\",1883,1\r\n","OK",2000)) //连接MQTT
						  if(ESP_SendAT("AT+MQTTSUB=0,\"iot/user4/channel2\",1\r\n","OK",2000)) //订阅
							{
									 return 1;
							}
	return 0;
}





