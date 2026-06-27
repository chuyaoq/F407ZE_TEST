#include "audioplay.h"
#include "wm8978.h"
#include "record.h"
#include "ff.h"
#include "stdio.h"

#include "i2s.h"
#include "fatfs.h"

__wavctrl wavctrl;

void Record_Play_Init(void)
{
	WM8978_ADDA_Cfg(0,1);		//?a??DAC 
	WM8978_Input_Cfg(0,0,0);	//1?±?ê?è?í¨µà(MIC&LINE IN)
	WM8978_Output_Cfg(1,0);		//?a??DACê?3? 
	WM8978_MIC_Gain(0);			//MIC??ò?éè???a0 
}

uint8_t Play_music(u8* pname)
{
	u8 res=Wav_Decode(pname,&wavctrl);
	if(res==0){
		if(wavctrl.bps==16){
			WM8978_I2S_Cfg(2,0);
			I2S2_Init(I2S_DATAFORMAT_16B_EXTENDED,wavctrl.samplerate);
		}else if(wavctrl.bps==24){
			WM8978_I2S_Cfg(2,2);
			I2S2_Init(I2S_DATAFORMAT_24B,wavctrl.samplerate);
		}
		res=f_open(&SDFile,(char*)pname,FA_READ);	//´ò¿ªWAVÒôÆµÎÄ¼þ
		if(res==RES_OK){
			f_lseek(&SDFile,wavctrl.datastart);
			return 0;	// ½âÎö³É¹¦
		}
	}
	return 1; // ½âÎöÊ§°Ü
}




