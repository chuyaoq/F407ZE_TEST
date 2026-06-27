#ifndef __AHT10_H
#define __AHT10_H

//AHT10温湿度结构体
typedef struct{
	float humidity;		//相对温度
	float temperature;	//相对湿度
}AHT_data;

void Aht10_init(void);
AHT_data Aht10_getdata(void);



#endif
