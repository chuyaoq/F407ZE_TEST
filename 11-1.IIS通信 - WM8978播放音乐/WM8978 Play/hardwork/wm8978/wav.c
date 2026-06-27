#include "wav.h"
#include "fatfs.h"
#include "stdio.h"
#include "string.h"

// __WaveHeader
void recoder_wav_init(__WaveHeader* wavhead) //初始化WAV头			   
{
	wavhead->riff.ChunkID=0X46464952;	//"RIFF"
	wavhead->riff.ChunkSize=0;			//还未确定,最后需要计算
	wavhead->riff.Format=0X45564157; 	//"WAVE"
	wavhead->fmt.ChunkID=0X20746D66; 	//"fmt "
	wavhead->fmt.ChunkSize=16; 			//大小为16个字节
	wavhead->fmt.AudioFormat=0X01; 		//0X01,表示PCM;0X01,表示IMA ADPCM
 	wavhead->fmt.NumOfChannels=2;		//双声道
 	wavhead->fmt.SampleRate=hi2sx.Init.AudioFreq;		//16Khz采样率 采样速率
 	wavhead->fmt.ByteRate=wavhead->fmt.SampleRate*4;//字节速率=采样率*通道数*(ADC位数/8)
 	wavhead->fmt.BlockAlign=4;			//块大小=通道数*(ADC位数/8)
 	wavhead->fmt.BitsPerSample=16;		//16位PCM
   	wavhead->data.ChunkID=0X61746164;	//"data"
 	wavhead->data.ChunkSize=0;			//数据大小,还需要计算  
}

u8 Wav_Decode(u8* pname,__wavctrl* wavx)
{
	ChunkRIFF *riff=(ChunkRIFF*)malloc(sizeof(ChunkRIFF));
	ChunkFMT *fmt=(ChunkFMT*)malloc(sizeof(ChunkFMT));
	ChunkDATA *data=(ChunkDATA*)malloc(sizeof(ChunkDATA));
	u8 ret;
	UINT br;
	FRESULT res=f_open(&SDFile,(TCHAR*)pname,FA_READ);
	if(res==FR_OK){
		wavx->datastart=sizeof(ChunkRIFF);
		f_read(&SDFile,riff,sizeof(ChunkRIFF),&br);
		if(riff->Format==0X45564157)//是WAV文件
		{
			wavx->datastart+=sizeof(ChunkFMT);
			f_read(&SDFile,fmt,sizeof(ChunkFMT),&br);
			if(fmt->ChunkID==0X20746D66){
				while(br){
					wavx->datastart+=sizeof(ChunkDATA);
					f_read(&SDFile,data,sizeof(ChunkDATA),&br);
					if(data->ChunkID==0X61746164)
					{
						break;
					}
					f_lseek(&SDFile,f_tell(&SDFile)+data->ChunkSize);
				}
				if(br){
					wavx->audioformat=fmt->AudioFormat;		//音频格式
					wavx->nchannels=fmt->NumOfChannels;		//通道数
					wavx->blockalign=fmt->BlockAlign;		//块对齐
					wavx->datasize=data->ChunkSize;			//数据块大小

					wavx->bitrate=fmt->ByteRate*8;			//得到位速
					wavx->samplerate=fmt->SampleRate;		//采样率
					wavx->bps=fmt->BitsPerSample;			//位数,16/24/32位
					
					wavx->datastart=wavx->datastart+8;		//数据流开始的地方. 
					wavx->played_num=0;						//已经播放的字节数清零
				}else
					ret=4;
			}else
				ret=3;
		}else
			ret=2;
		f_close(&SDFile);
	}else{
		printf("Decode file %s error: %d\r\n",pname,res);
		ret=1;	// 文件打开失败
	}
	free(riff);
	free(fmt);
	free(data);
	return ret;
}

u32 wav_buffill(u8 *buf,u16 size,u8 bits)
{
	u32 bread=0;
	u8 br;
	u16 i;
	if(bits==24)//24bit音频,需要处理一下
	{
		memset(buf,0,size);
		for(i=0;i<size;)
		{
			f_read(&SDFile,buf+i+1,2,(UINT*)&br);	//读取数据
			if(!br) break;
			bread+=br;
			f_read(&SDFile,buf+i+3,1,(UINT*)&br);
			if(!br) break;
			bread+=br;
			i+=4;
		}
		bread=bread/3;
		if(i<size)//不够数据了,补充0
		{
			for(;i<size;i++)buf[i]=0; 
		}
	}else 
	{
		f_read(&SDFile,buf,size,(UINT*)&bread);//16bit音频,直接读取数据  
		if(bread<size)//不够数据了,补充0
		{
			for(i=bread;i<size-bread;i++)buf[i]=0; 
		}
	}
	return bread;
}


