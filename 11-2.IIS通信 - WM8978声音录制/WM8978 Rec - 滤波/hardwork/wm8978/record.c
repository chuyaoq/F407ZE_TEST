#include "record.h"
#include "wm8978.h"

void Record_Rec_Init(void)
{
	WM8978_ADDA_Cfg(1,0);		//开启ADC
	WM8978_Input_Cfg(1,0,0);	//开启输入通道(MIC&LINE IN)
	WM8978_Output_Cfg(0,1);		//开启BYPASS输出 
	WM8978_MIC_Gain(46);		//MIC增益设置 
	WM8978_I2S_Cfg(2,0);		//飞利浦标准,16位数据长度
}

