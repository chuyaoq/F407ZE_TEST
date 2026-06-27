#ifndef __ESP_H__
#define __ESP_H__
#include "main.h"

uint8_t Find_Str(uint8_t* Str1, uint8_t* Str2);
uint8_t ESP_SendAT(uint8_t *ch,uint8_t *Restr,uint16_t timeout);
uint8_t BLE_Set(void);  //WIFI初始化设置
void Uart3_send(uint8_t *ch);
#endif

