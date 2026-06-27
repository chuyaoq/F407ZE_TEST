#ifndef __WAV_H_
#define __WAV_H_

#include "main.h"
#include "i2s.h"

#define hi2sx hi2s2

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef __packed struct
{
	u32 ClockID;
	u32 ClockSize;
}ChunkClock;

//RIFF块
typedef __packed struct
{
    u32 ChunkID;		   	//chunk id;这里固定为"RIFF",即0X46464952
    u32 ChunkSize ;		   	//集合大小;文件总大小-8
    u32 Format;	   			//格式;WAVE,即0X45564157
}ChunkRIFF ;
//fmt块
typedef __packed struct
{
    u32 ChunkID;		   	//chunk id;这里固定为"fmt ",即0X20746D66
    u32 ChunkSize ;		   	//子集合大小(不包括ID和Size);这里为:20.
    u16 AudioFormat;	  	//音频格式;0X01,表示线性PCM;0X11表示IMA ADPCM
	u16 NumOfChannels;		//通道数量;1,表示单声道;2,表示双声道;
	u32 SampleRate;			//采样率;0X1F40,表示8Khz
	u32 ByteRate;			//字节速率; 
	u16 BlockAlign;			//块对齐(字节); 
	u16 BitsPerSample;		//单个采样数据大小;4位ADPCM,设置为4
}ChunkFMT;	   

//data块 
typedef __packed struct 
{
    u32 ChunkID;		   	//chunk id;这里固定为"data",即0X5453494C
    u32 ChunkSize ;		   	//子集合大小(不包括ID和Size) 
}ChunkDATA;

//wav头
typedef __packed struct
{ 
	ChunkRIFF riff;	//riff块
	ChunkFMT fmt;  	//fmt块
	ChunkDATA data;	//data块		 
}__WaveHeader;

//wav 播放控制结构体
typedef __packed struct
{ 
    u16 audioformat;			//音频格式;0X01,表示线性PCM;0X11表示IMA ADPCM
	u16 nchannels;				//通道数量;1,表示单声道;2,表示双声道; 
	u16 blockalign;				//块对齐(字节);  
	u32 datasize;				//WAV数据大小 

	
    u32 bitrate;	   			//比特率(位速)
	u32 samplerate;				//采样率 
	u16 bps;					//位数,比如16bit,24bit,32bit

	u32 played_num;				//已经播放的字节数
}__wavctrl; 

void recoder_wav_init(__WaveHeader* wavhead);

#endif

