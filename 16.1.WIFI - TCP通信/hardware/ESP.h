#ifndef __ESP_H__
#define __ESP_H__
#include "main.h"

uint8_t check_Buff(uint8_t* buff1,uint8_t len1, uint8_t* buff2, uint8_t len2);
uint8_t Find_Str(uint8_t* Str1, uint8_t* Str2);
uint8_t ESP_SendAT(uint8_t *ch,uint8_t *Restr,uint16_t timeout);
uint8_t WIFI_Set(void);  //WIFI初始化设置
uint8_t WIFI_Netconnect(uint8_t *fun_IP);  // IP地址连接
void WIFI_deal(void);
void Uart3_send(uint8_t *ch);
uint8_t WIFI_Senddat(uint8_t type ,uint8_t *str,uint16_t lenth);
void AP_get(void);
#endif

