#ifndef __WM8978_H
#define __WM8978_H

#include "main.h"

typedef uint8_t u8;
typedef uint16_t u16;

//如果AD0脚(4脚)接地,IIC地址为0X4A(不包含最低位).
//如果接V3.3,则IIC地址为0X4B(不包含最低位).
#define WM8978_ADDR				0X1A	//WM8978的器件地址,固定为0X1A 
 
//#define EQ1_80Hz		0X00
//#define EQ1_105Hz		0X01
//#define EQ1_135Hz		0X02
//#define EQ1_175Hz		0X03

//#define EQ2_230Hz		0X00
//#define EQ2_300Hz		0X01
//#define EQ2_385Hz		0X02
//#define EQ2_500Hz		0X03

//#define EQ3_650Hz		0X00
//#define EQ3_850Hz		0X01
//#define EQ3_1100Hz		0X02
//#define EQ3_14000Hz		0X03

//#define EQ4_1800Hz		0X00
//#define EQ4_2400Hz		0X01
//#define EQ4_3200Hz		0X02
//#define EQ4_4100Hz		0X03

//#define EQ5_5300Hz		0X00
//#define EQ5_6900Hz		0X01
//#define EQ5_9000Hz		0X02
//#define EQ5_11700Hz		0X03

  
u8 WM8978_Init(void); 

//WM8978 DAC/ADC配置
//adcen:adc使能(1)/关闭(0)
//dacen:dac使能(1)/关闭(0)
void WM8978_ADDA_Cfg(u8 adcen,u8 dacen);

//WM8978 输入通道配置 
//micen:MIC开启(1)/关闭(0)
//lineinen:Line In开启(1)/关闭(0)
//auxen:aux开启(1)/关闭(0) 
void WM8978_Input_Cfg(u8 micen,u8 lineinen,u8 auxen);

//WM8978 输出配置 
//dacen:DAC输出(放音)开启(1)/关闭(0)
//bpsen:Bypass输出(录音,包括MIC,LINE IN,AUX等)开启(1)/关闭(0) 
void WM8978_Output_Cfg(u8 dacen,u8 bpsen);

//WM8978 MIC增益设置(不包括BOOST的20dB,MIC-->ADC输入部分的增益)
//gain:0~63,对应-12dB~35.25dB,0.75dB/Step
void WM8978_MIC_Gain(u8 gain);

//WM8978 L2/R2(也就是Line In)增益设置(L2/R2-->ADC输入部分的增益)
//gain:0~7,0表示通道禁止,1~7,对应-12dB~6dB,3dB/Step
void WM8978_LINEIN_Gain(u8 gain);

//WM8978 AUXR,AUXL(PWM音频部分)增益设置(AUXR/L-->ADC输入部分的增益)
//gain:0~7,0表示通道禁止,1~7,对应-12dB~6dB,3dB/Step
void WM8978_AUX_Gain(u8 gain);


u8 WM8978_Write_Reg(u8 reg,u16 val);

u16 WM8978_Read_Reg(u8 reg);

//设置耳机左右声道音量
//voll:左声道音量(0~63)
//volr:右声道音量(0~63)
void WM8978_HPvol_Set(u8 voll,u8 volr);

//设置喇叭音量
//voll:左声道音量(0~63) 
void WM8978_SPKvol_Set(u8 volx);

//设置3D环绕声
//depth:0~15(3D强度,0最弱,15最强)
void WM8978_3D_Set(u8 depth);

//设置I2S工作模式
//fmt:0,LSB(右对齐);1,MSB(左对齐);2,飞利浦标准I2S;3,PCM/DSP;
//len:0,16位;1,20位;2,24位;3,32位;  
void WM8978_I2S_Cfg(u8 fmt,u8 len);

//设置EQ/3D作用方向
//dir:0,在ADC起作用
//    1,在DAC起作用(默认)
void WM8978_EQ_3D_Dir(u8 dir); 

//设置EQ1
//cfreq:截止频率,0~3,分别对应:80/105/135/175Hz
//gain:增益,0~24,对应-12~+12dB
void WM8978_EQ1_Set(u8 cfreq,u8 gain);

//设置EQ2
//cfreq:中心频率,0~3,分别对应:230/300/385/500Hz
//gain:增益,0~24,对应-12~+12dB
void WM8978_EQ2_Set(u8 cfreq,u8 gain);

//设置EQ3
//cfreq:中心频率,0~3,分别对应:650/850/1100/1400Hz
//gain:增益,0~24,对应-12~+12dB
void WM8978_EQ3_Set(u8 cfreq,u8 gain);

//设置EQ4
//cfreq:中心频率,0~3,分别对应:1800/2400/3200/4100Hz
//gain:增益,0~24,对应-12~+12dB
void WM8978_EQ4_Set(u8 cfreq,u8 gain);

//设置EQ5
//cfreq:中心频率,0~3,分别对应:5300/6900/9000/11700Hz
//gain:增益,0~24,对应-12~+12dB
void WM8978_EQ5_Set(u8 cfreq,u8 gain);

#endif





















