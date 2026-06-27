#ifndef __SCCB_H
#define __SCCB_H

#include "main.h"

//IO方向设置
#define SCCB_SDA_IN()  {GPIOB->MODER&=~(3<<(9*2));GPIOB->MODER|=0<<9*2;}	//PD7 输入
#define SCCB_SDA_OUT() {GPIOB->MODER&=~(3<<(9*2));GPIOB->MODER|=1<<9*2;} 	//PD7 输出

#define SCCB_ID   			0X60  			//OV2640的ID

#define CPU_FREQUENCY_MHZ 168				/* CPU主频，根据实际进行修改 */


///////////////////////////////////////////
void delay_us(uint32_t delay);

void SCCB_Init(void);
void SCCB_Start(void);
void SCCB_Stop(void);
void SCCB_No_Ack(void);
uint8_t SCCB_WR_Byte(uint8_t dat);
uint8_t SCCB_RD_Byte(void);
uint8_t SCCB_WR_Reg(uint8_t reg,uint8_t data);
uint8_t SCCB_RD_Reg(uint8_t reg);


#endif
