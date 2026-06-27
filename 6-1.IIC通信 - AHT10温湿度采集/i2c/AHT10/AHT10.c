#include "main.h"
#include "stdio.h"
#include "i2c.h"
#include "AHT10.h"


// 定义AHT10命令
#define AHT10_INIT 0xE1// 1110 0001
#define AHT10_REST 0xBA// 1011 1010
#define AHT10_TRIG 0xAC// 1010 1100

// 定义AHT10地址
#define AHT10_Read 0x71	// 0x38<<1 | 1
#define AHT10_Write 0x70// 0x38<<1 | 0



/**
 * @brief  AHT10 初始化
 * @param  void
 * @retval void
 */
void Aht10_init(void) {
	uint8_t date=AHT10_INIT;
	HAL_I2C_Master_Transmit(&hi2c2,AHT10_Write,&date,1,0xffff);
}

/**
 * @brief  AHT10 软复位
 * @param  void
 * @retval void
 */
void Aht10_Rst(void) {
	uint8_t date=AHT10_REST;
	HAL_I2C_Master_Transmit(&hi2c2,AHT10_Write,&date,1,0xffff);
	 HAL_Delay(20);
}
/**
 * @brief  AHT10触发测量
 * @param  void
 * @retval void
 */
void Aht10_Trig(void) {
	uint8_t date[3];
	date[0]=AHT10_TRIG;
	date[1]=0x33;
	date[2]=0x00;
	HAL_I2C_Master_Transmit(&hi2c2,AHT10_Write,date,3,0xffff);
}
/**
 * @brief  AHT10 设备读取 相对湿度和温度
 * @param  void
 * @retval AHT_data 有数据代表读取数据正常; NULL-读取设备失败，设备一直处于忙状态，不能获取数据
 */
AHT_data Aht10_getdata(void) {
	AHT_data sensor_data;
	uint8_t data[6];
	uint8_t busy,cal;
	
	// 初始化返回值为空
	sensor_data.humidity = NULL;
    sensor_data.temperature = NULL;
	
	// 1 发送测量触发命令
	Aht10_Trig();
	
	// 2 等待测量完成
	HAL_Delay(100);	
	
	// 3 读取AHT10传输的6字节数据
	HAL_I2C_Master_Receive(&hi2c2,AHT10_Read,data,6,0xffff);    

	
	
	// 4 取出状态位
	busy=(data[0]>>7)&0x01;// 状态标志位
	cal=(data[0]>>3)&0x01;// 校准标志位
	
	// 5 检查校准状态
	if(cal==0){
		 // 传感器未校准，复位传感器
		Aht10_Rst();
		return sensor_data;
	}
	
	// 6 检查数据是否有效
	if(busy==0){// 数据有效
		
		// 进行数据转换
		uint32_t temp,humi;
		
		// 取出数据
		humi=(data[1]<<12)|(data[2]<<4)|((data[3]>>4)&0x0f);
		temp=((data[3]& 0x0F)<<16)|(data[4]<<8)|(data[5]);
		
		// 转换为实际温度和湿度值
		sensor_data.humidity=(humi*100.0)/1048576;
		sensor_data.temperature=((float)temp/1048576)*200-50;
		
		// 返回包含有效数据的结构体
		return sensor_data;
	}else
		// 数据无效
		return sensor_data;
}






