#include "gpio.h"
#include "sx1262.h"
#include "usart.h"
#include "spi.h"
FlagType  Sx1262_Flag;  // SX1262状态标志位结构体，存放各类工作状态和超时标志

/**
  * @brief  SPI总线读写函数(半双工)
  * @param  data ：需要通过SPI发送的字节数据
  * @retval 接收到的从机返回字节数据
  * @note   SX1262与MCU之间通过SPI3通信，收发一体，发送1字节同时接收1字节
  */
uint8_t spi_rw(uint8_t data) 
{
	uint8_t rdata=0x00;
	// SPI3收发1字节数据，超时时间1000ms
	HAL_SPI_TransmitReceive(&hspi3, &data, &rdata, 1,1000);
	return (rdata);	
}

/***************SX1262 核心驱动函数区 START*****************/

/**
  * @brief  检测SX1262的BUSY引脚电平，等待芯片空闲
  * @param  None
  * @retval None
  * @note   1.BUSY引脚为高电平时，表示芯片正在忙(收发/配置/待机)，无法进行新的SPI指令操作
  *         2.超时后会自动复位芯片+重新配置+恢复接收，防止芯片卡死
  */
void check_busy(void)
{
	// 清空超时计数和超时标志位
	Sx1262_Flag.busy_timeout_cnt = 0;								
	Sx1262_Flag.busy_timeout = 0;			
	// 循环检测BUSY引脚，高电平=忙，等待至空闲
	while(HAL_GPIO_ReadPin(LORA1_BUSY_GPIO_Port, LORA1_BUSY_Pin)==SET)
	{
		// 检测到BUSY超时，执行芯片异常恢复流程
		if(Sx1262_Flag.busy_timeout)			
		{
			SetStandby(0);        // 进入待机RC模式，低功耗待机
			reset_sx1262();	    // 硬件复位射频芯片
			sx1262_Config();     // 重新配置射频参数
			Rx_Init();           // 重新初始化接收模式
			break;		         // 退出等待循环
		}
	}
}

/**
  * @brief  SX1262 硬件复位函数
  * @param  None
  * @retval None
  * @note   复位时序：拉低RST引脚→延时(大于100us)→拉高RST引脚→稳定延时
  *         复位后芯片恢复默认状态，必须重新配置参数才能工作
  */
void reset_sx1262(void)
{
	// 拉低复位引脚
	HAL_GPIO_WritePin(LORA1_RST_GPIO_Port, LORA1_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(2);		 // 延时2ms，满足芯片复位最小低电平时间(>100us)
	// 拉高复位引脚，完成复位
	HAL_GPIO_WritePin(LORA1_RST_GPIO_Port, LORA1_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(20);		 // 延时20ms，等待芯片内部电路稳定
} 

/**
  * @brief  设置SX1262进入睡眠模式
  * @param  None
  * @retval None
  * @note   睡眠模式是SX1262最低功耗模式，SPI仍可通信，唤醒最快
  *         sleepConfig=0x04：bit2=1 热启动(保留配置)，bit0=0 关闭RTC超时唤醒
  */
void SetSleep(void)
{
	uint8_t Opcode,sleepConfig;
	
	check_busy();                // 等待芯片空闲
	Opcode = SET_SLEEP;	        // 睡眠模式指令码 0x84
	sleepConfig = 0x04;	        // 睡眠配置参数：热启动+关闭RTC唤醒
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET); // SPI片选拉低，选中芯片
	spi_rw(Opcode);             // 发送睡眠指令
	spi_rw(sleepConfig);        // 发送睡眠配置参数
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);   // SPI片选拉高，释放芯片
}

/**
  * @brief  设置SX1262进入待机模式
  * @param  StdbyConfig ：待机模式配置参数
  *         @arg 0 ：STDBY_RC  基于RC振荡器待机，低功耗，精度低
  *         @arg 1 ：STDBY_XOSC 基于晶振待机，功耗稍高，频率精度高(收发必选)
  * @retval None
  */
void SetStandby(uint8_t StdbyConfig)
{
	uint8_t Opcode;
	
	check_busy();               // 等待芯片空闲
	Opcode = SET_STANDBY;	    // 待机模式指令码 0x80	
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(StdbyConfig);
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  设置SX1262进入发送模式
  * @param  timeout ：发送超时时间，单位：ms，设置0表示永不超时
  * @retval None
  * @note   发送指令后，芯片自动完成数据包发送，完成后触发TxDone中断
  *         超时参数为3字节数据，按 高字节→中字节→低字节 顺序发送
  */
void SetTx(uint32_t timeout)
{
	uint8_t Opcode;
	uint8_t time_out[3];
	
	check_busy();
	Opcode = SET_TX;	        // 发送模式指令码 0x83
	// 将32位超时值拆分为3个8位字节，高位在前
	time_out[0] = (timeout>>16)&0xFF; // MSB 最高位
	time_out[1] = (timeout>>8)&0xFF;
	time_out[2] = timeout&0xFF;       // LSB 最低位
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(time_out[0]);
	spi_rw(time_out[1]);
	spi_rw(time_out[2]);
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  设置SX1262进入接收模式
  * @param  timeout ：接收超时时间，单位：ms，设置0表示永不超时
  * @retval None
  * @note   接收指令后，芯片持续监听空中数据包，收到后触发RxDone中断
  *         超时参数为3字节数据，按 高字节→中字节→低字节 顺序发送
  */
void SetRx(uint32_t timeout)
{
	uint8_t Opcode;
	uint8_t time_out[3];
	
	check_busy();

	Opcode = SET_RX;	        // 接收模式指令码 0x82
	// 将32位超时值拆分为3个8位字节，高位在前
	time_out[0] = (timeout>>16)&0xFF;
	time_out[1] = (timeout>>8)&0xFF;
	time_out[2] = timeout&0xFF;
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(time_out[0]);
	spi_rw(time_out[1]);
	spi_rw(time_out[2]);
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  设置SX1262的数据包调制类型
  * @param  PacketType ：调制类型配置参数
  *         @arg 0 ：GFSK 高斯频移键控(传统射频模式)
  *         @arg 1 ：LORA LoRa扩频调制(远距离低速率核心模式)
  * @retval None
  */
void SetPacketType(uint8_t PacketType)
{
	uint8_t Opcode;
	
	check_busy();
	Opcode = SET_PACKET_TYPE;	// 数据包类型指令码 0x8A	
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(PacketType);
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  读取SX1262当前配置的数据包调制类型
  * @param  None
  * @retval 0=GFSK模式，1=LORA模式
  */
uint8_t GetPacketType(void)
{
	uint8_t Opcode;
	uint8_t packetType;
	
	check_busy();
	Opcode = 0x11;	// 读取数据包类型指令码	
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(0xFF);     // 读取状态字节(占位)
	packetType = spi_rw(0xFF); // 读取实际的调制类型参数
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
	
	return packetType;
}

/**
  * @brief  设置SX1262的射频工作频率
  * @param  RfFreq ：需要配置的实际射频频率，单位：Hz (例：470000000 = 470MHz)
  * @retval None
  * @note   频率计算公式：RF_Freq = freq_reg * 32M / (2^25)
  *         反算寄存器值：freq_reg = (RF_Freq * (2^25)) / 32000000
  *         配置的频率必须在SX1262的工作频段内(410~525MHz)
  */
void SetRfFrequency(uint32_t RfFreq)
{                                                                         
	uint8_t Opcode;
	uint8_t Rf_Freq[4];
    // 计算频率寄存器值，强制类型转换防止溢出
    RfFreq = (uint32_t) ((((uint64_t) RfFreq)<<25)/(32000000) );
	check_busy();
	
	Opcode = SET_RF_FREQUENCY;	// 设置射频频率指令码 0x86
	
	// 将计算后的频率值拆分为4个8位字节，高位在前
	Rf_Freq[0] = (RfFreq>>24)&0xFF;
	Rf_Freq[1] = (RfFreq>>16)&0xFF;
	Rf_Freq[2] = (RfFreq>>8)&0xFF;
	Rf_Freq[3] = RfFreq&0xFF;
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(Rf_Freq[0]);
	spi_rw(Rf_Freq[1]);
	spi_rw(Rf_Freq[2]);
	spi_rw(Rf_Freq[3]);
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  设置SX1262的功率放大器PA配置
  * @param  None
  * @retval None
  * @note   固定配置：高功率模式，最大22dBm，适配SX1262芯片
  *         0x04=PA占空比，0x07=最大功率等级(22dBm)，0x00=芯片型号SX1262
  */
void SetPaConfig(void)
{
	uint8_t Opcode;
	
	check_busy();
	Opcode = 0x95;	// PA配置指令码	
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(0x04);	// paDutyCycle 功率放大器占空比
	spi_rw(0x07);	// hpMax 最大功率等级 0~7，7对应22dBm
	spi_rw(0x00);	// deviceSel 芯片型号选择 0=SX1262，1=SX1261
	spi_rw(0x01);	// 保留参数
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  设置SX1262的稳压器工作模式
  * @param  None
  * @retval None
  * @note   0x01=DC-DC开关稳压器模式，效率高，推荐收发模式使用
  */
void SetRegulatorMode(void)
{
	uint8_t Opcode;
	
	check_busy();
	Opcode = 0x96;	// 稳压器配置指令码	
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(0x01);   // regModeParam 0x01=DC-DC模式，0x00=LDO模式
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/*
TX功率参数说明：
- 低功率PA：功率范围 -17(0xEF) ~ +14(0x0E) dBm，步进1dB
- 高功率PA：功率范围  -9(0xF7) ~ +22(0x16) dBm，步进1dB

射频功率爬升时间RampTime参数定义：
-------------------------------------
RampTime宏定义  | 数值  | 实际爬升时间(µs)
-------------------------------------
SET_RAMP_10U    | 0x00  | 10
SET_RAMP_20U    | 0x01  | 20
SET_RAMP_40U 	| 0x02	| 40
SET_RAMP_80U 	| 0x03	| 80
SET_RAMP_200U 	| 0x04	| 200
SET_RAMP_800U 	| 0x05	| 800
SET_RAMP_1700U 	| 0x06	| 1700
SET_RAMP_3400U 	| 0x07	| 3400
*/
#define PACK_MAX_LEN 32  // 定义最大数据包长度 32字节

/**
  * @brief  设置SX1262的发送参数(发射功率+功率爬升时间)
  * @param  power ：发射功率值，参考上方功率参数说明
  * @param  RampTime ：功率爬升时间，参考上方RampTime参数定义
  * @retval None
  */
void SetTxParams(uint8_t power,uint8_t RampTime)
{
	uint8_t Opcode;
	
	check_busy();
	Opcode = SET_TX_PARAMS;	// 发送参数配置指令码 0x8E	
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(power);       // 配置发射功率
	spi_rw(RampTime);    // 配置功率爬升时间
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin,GPIO_PIN_SET);
}

/**
  * @brief  设置SX1262内部FIFO缓冲区的基地址
  * @param  TX_base_addr ：发送缓冲区起始地址(0~255)
  * @param  RX_base_addr ：接收缓冲区起始地址(0~255)
  * @retval None
  * @note   配置0,0表示收发都从FIFO首地址开始读写，简化操作
  */
void SetBufferBaseAddress(uint8_t TX_base_addr,uint8_t RX_base_addr)
{
	uint8_t Opcode;
	
	check_busy();
	Opcode = SET_BUF_BASE_ADDR;	// FIFO地址配置指令码 0x8F	
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(TX_base_addr);
	spi_rw(RX_base_addr);
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  写SX1262的寄存器
  * @param  address ：寄存器地址(16位)
  * @param  data ：待写入的寄存器数据缓存区首地址
  * @param  length ：写入的字节长度
  * @retval None
  */
void WriteRegister(uint16_t address, uint8_t *data, uint8_t length)
{
	uint8_t Opcode;
	uint8_t addr_l,addr_h;
	uint8_t i;
	
	if(length<1)	return;  // 写入长度为0，直接退出
	
	check_busy();
	addr_l = address&0xff;  // 拆分寄存器地址为低8位
	addr_h = address>>8;    // 拆分寄存器地址为高8位
	Opcode = 0x0D;          // 写寄存器指令码
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(addr_h);         // 先发地址高字节
	spi_rw(addr_l);         // 再发地址低字节
	for(i=0;i<length;i++)   // 循环写入数据
	{
		spi_rw(data[i]);
	}
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  读SX1262的寄存器
  * @param  address ：寄存器地址(16位)
  * @param  data ：存储读取数据的缓存区首地址
  * @param  length ：读取的字节长度
  * @retval None
  */
void ReadRegister(uint16_t address, uint8_t *data, uint8_t length)
{
	uint8_t Opcode;
	uint8_t addr_l,addr_h;
	uint8_t i;
	
	if(length<1)	return;  // 读取长度为0，直接退出
	check_busy();
	
	addr_l = address&0xff;
	addr_h = address>>8;
	
	Opcode = 0x1D;          // 读寄存器指令码
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(addr_h);
	spi_rw(addr_l);
	for(i=0;i<length;i++)   // 循环读取数据
	{
		data[i]=spi_rw(0xFF);
	}
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  写SX1262的FIFO发送缓冲区
  * @param  offset ：相对于基地址的偏移地址
  * @param  data ：待发送的数据缓存区首地址
  * @param  length ：发送数据的字节长度
  * @retval None
  */
void WriteBuffer(uint8_t offset, uint8_t *data, uint8_t length)
{
	uint8_t Opcode;
	uint8_t i;
	
	if(length<1)	return;
	
	check_busy();
	Opcode = 0x0E;  // 写FIFO指令码
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(offset); // 写入偏移地址
	for(i=0;i<length;i++)
	{
		spi_rw(data[i]);
	}
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  读SX1262的FIFO接收缓冲区
  * @param  offset ：相对于基地址的偏移地址
  * @param  data ：存储接收数据的缓存区首地址
  * @param  length ：读取数据的字节长度
  * @retval None
  */
void ReadBuffer(uint8_t offset, uint8_t *data, uint8_t length)
{
	uint8_t Opcode;
	uint8_t i;
	
	if(length<1)	return;
	check_busy();
	
	Opcode = 0x1E;  // 读FIFO指令码
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(offset); // 读取偏移地址
	spi_rw(0xFF);   // 占位字节
	for(i=0;i<length;i++)
	{
		data[i]=spi_rw(0xFF);
	}
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  获取接收缓冲区状态
  * @param  payload_len ：输出参数，接收到的数据包有效长度
  * @param  buf_pointer ：输出参数，接收数据在FIFO中的首地址偏移量
  * @retval None
  */
void GetRxBufferStatus(uint8_t *payload_len, uint8_t *buf_pointer)
{
	uint8_t Opcode;
	
	check_busy();
	
	Opcode = 0x13;  // 读取接收状态指令码
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);

	spi_rw(0xFF);
	*payload_len = spi_rw(0xFF);  // 获取有效数据长度
	*buf_pointer = spi_rw(0xFF);  // 获取数据首地址偏移
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  设置LoRa调制解调参数
  * @param  sf ：扩频因子，取值5~12 (SF7~SF12)
  * @param  bw ：带宽，LORA_BW_125/250/500 对应125/250/500KHz
  * @param  cr ：编码率，LORA_CR_4_5/4_6/4_7/4_8
  * @param  ldro ：低速优化使能 0=关闭 1=开启
  * @retval None
  * @note   调制参数决定了通信距离和速率，SF越大/带宽越小，距离越远，速率越低
  */
void SetModulationParams(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro)
{
	uint8_t Opcode;
	
	check_busy();
	Opcode = 0x8B;  // 调制参数配置指令码
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	
	spi_rw(sf);     // 扩频因子 SF=5~12
	spi_rw(bw);     // 带宽 BW
	spi_rw(cr);     // 编码率 CR
	spi_rw(ldro);   // 低速优化 LDRO 0=关，1=开
	
	spi_rw(0XFF);   // 保留参数，填0xFF
	spi_rw(0XFF);
	spi_rw(0XFF);
	spi_rw(0XFF);
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  设置LoRa数据包参数
  * @param  payload_len ：数据包有效长度
  * @retval None
  * @note   固定配置：8字节前导码、显式头部、CRC校验开启、标准IQ信号
  */
void SetPacketParams(uint8_t payload_len)
{
	uint8_t Opcode;
	uint16_t prea_len;
	uint8_t prea_len_h,prea_len_l;
	
	check_busy();
	
	Opcode = 0x8C;  // 数据包参数配置指令码
	
	prea_len = 8;   // 前导码长度8字节，保证高接收成功率
	prea_len_h = prea_len>>8;
	prea_len_l = prea_len&0xFF;
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	
	spi_rw(prea_len_h);  // 前导码长度高字节
	spi_rw(prea_len_l);  // 前导码长度低字节
	spi_rw(0x00);        // 头部类型 0=可变头部(显式)，1=固定头部(隐式)
	spi_rw(payload_len); // 有效载荷长度 0~0xFF
	
	spi_rw(0X01);        // CRC校验 1=开启，0=关闭
	spi_rw(0X00);        // IQ信号 0=标准模式，1=反转模式
	spi_rw(0XFF);        // 保留参数
	spi_rw(0XFF);
	spi_rw(0XFF);
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  设置SX1262的中断映射参数
  * @param  irq ：需要使能的中断类型(中断掩码)
  * @retval None
  * @note   本代码配置：所有使能的中断都映射到DIO1引脚，DIO2/DIO3无中断
  */
void SetDioIrqParams(uint16_t irq)
{
	uint8_t Opcode;
	uint16_t Irq_Mask;
	uint8_t Irq_Mask_h,Irq_Mask_l;
	uint16_t DIO1Mask;
	uint8_t DIO1Mask_h,DIO1Mask_l;
	uint16_t DIO2Mask;
	uint8_t DIO2Mask_h,DIO2Mask_l;
	uint16_t DIO3Mask;
	uint8_t DIO3Mask_h,DIO3Mask_l;
	
	Irq_Mask = irq;       // 使能的中断掩码
	DIO1Mask = irq;       // 所有中断映射到DIO1
	DIO2Mask = 0;         // DIO2无中断映射
	DIO3Mask = 0;         // DIO3无中断映射
	
	// 拆分16位中断掩码为高低字节
	Irq_Mask_h = Irq_Mask>>8;
	Irq_Mask_l = Irq_Mask&0xFF;
	DIO1Mask_h = DIO1Mask>>8;
	DIO1Mask_l = DIO1Mask&0xFF;
	DIO2Mask_h = DIO2Mask>>8;
	DIO2Mask_l = DIO2Mask&0xFF;
	DIO3Mask_h = DIO3Mask>>8;
	DIO3Mask_l = DIO3Mask&0xFF;
	
	Opcode = 0x08;  // 中断配置指令码
	check_busy();
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	
	spi_rw(Irq_Mask_h);
	spi_rw(Irq_Mask_l);
	spi_rw(DIO1Mask_h);
	spi_rw(DIO1Mask_l);
	
	spi_rw(DIO2Mask_h);
	spi_rw(DIO2Mask_l);
	spi_rw(DIO3Mask_h);
	spi_rw(DIO3Mask_l);
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  获取当前中断状态
  * @param  None
  * @retval 16位中断状态值，对应位为1表示对应中断触发
  */
uint16_t GetIrqStatus(void)
{
	uint8_t Opcode;
	uint16_t IrqStatus;
	uint8_t temp;
	
	check_busy();
	
	Opcode = 0x12;  // 读取中断状态指令码
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(0xFF);
	temp = spi_rw(0xFF);
	IrqStatus = temp;
	IrqStatus = IrqStatus<<8;
	temp = spi_rw(0xFF);
	IrqStatus = IrqStatus|temp;
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
	
	return IrqStatus;
}

/**
  * @brief  清除指定的中断标志位
  * @param  irq ：需要清除的中断掩码
  * @retval None
  * @note   中断触发后必须手动清除，否则会一直触发
  */
void ClearIrqStatus(uint16_t irq)
{
	uint8_t Opcode;
	uint16_t irq_h,irq_l;
	
	check_busy();
	
	irq_h = irq>>8;
	irq_l = irq&0xFF;
	
	Opcode = 0x02;  // 清除中断指令码
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(irq_h);
	spi_rw(irq_l);
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  配置DIO2引脚为射频开关控制引脚
  * @param  None
  * @retval None
  * @note   0x01=DIO2引脚自动控制射频天线开关，TX时为高，RX时为低
  */
void SetDIO2AsRfSwitchCtrl(void)
{
	uint8_t Opcode;
	check_busy();
	Opcode = 0x9D;  // DIO2配置指令码
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(0x01);   // DIO2控制射频开关，TX模式输出高电平
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  配置DIO3引脚为TCXO晶振控制引脚
  * @param  tcxoVoltage ：TCXO晶振供电电压
  * @retval None
  */
void SetDIO3AsTCXOCtrl(uint8_t tcxoVoltage)
{
	uint8_t Opcode;
	
	check_busy();
	Opcode = 0x97;  // DIO3配置指令码
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	spi_rw(tcxoVoltage);   // TCXO供电电压配置
	spi_rw(0x00);		   // 超时时间高字节
	spi_rw(0x00);
	spi_rw(0x64);          // 超时时间低字节，单位15.625us
	
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
}

uint8_t buf[4]={0};  // RSSI值缓存数组
/**
  * @brief  获取当前实时的RSSI值(信号强度指示)
  * @param  None
  * @retval 当前RSSI值，数值越小表示信号越强
  */
uint8_t GetRssiInst(void)
{
	uint8_t Opcode;
	uint8_t rssi;
	check_busy();
	Opcode = 0x14;  // 读取RSSI指令码
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_RESET);
	spi_rw(Opcode);
	buf[0] = spi_rw(0xFF);
	buf[1] = spi_rw(0xFF);
	buf[2] = spi_rw(0xFF);
	buf[3] = spi_rw(0xFF);
	HAL_GPIO_WritePin(LORA1_CS_GPIO_Port, LORA1_CS_Pin, GPIO_PIN_SET);
	// 根据当前射频模式，读取对应RSSI值
	if(Sx1262_Flag.RFCtrl.RFmode)
		rssi = buf[1];  // LoRa模式取buf[1]
	else
		rssi = buf[3];  // GFSK模式取buf[3]
	return rssi;
}

/**
  * @brief  启动SX1262发送数据包
  * @param  txbuf ：待发送的数据缓存区首地址
  * @param  payload_length ：发送数据的有效字节长度
  * @retval None
  * @note   发送完成后自动清除发送中断标志，异常超时会复位芯片
  */
void Tx_Start(uint8_t *txbuf,uint8_t payload_length)
{
	SetStandby(1);                          // 进入晶振待机模式，保证发送精度
	SetBufferBaseAddress(0,0);              // 设置FIFO收发基地址为0
	WriteBuffer(0,txbuf,payload_length);    // 将发送数据写入FIFO
	SetPacketParams(payload_length);        // 配置数据包参数

	SetDioIrqParams(TxDone_IRQ);            // 使能发送完成中断

	SetTx(0);                               // 启动发送模式，永不超时
	
	Sx1262_Flag.is_tx = 1;                  // 设置发送状态标志位

	Sx1262_Flag.rf_timeout = 0;							
	Sx1262_Flag.rf_reach_timeout = 0;		// 清空发送超时标志位
	
	// 等待发送完成中断或超时
	while(!RF_IRQ())
	{
		// 发送超时，执行异常恢复
		if(Sx1262_Flag.rf_reach_timeout)			
		{
			ClearIrqStatus(TxDone_IRQ);    // 清除发送完成中断标志
			SetStandby(0);                // 进入RC待机模式
			reset_sx1262();	            // 复位射频芯片
			sx1262_Config();             // 重新配置参数
			break;		                 // 退出等待循环
		}
		Sx1262_Flag.Irq_Status = GetIrqStatus();  // 读取中断状态
		if(TxDone_IRQ&Sx1262_Flag.Irq_Status)break; // 判断发送完成中断是否触发
	}
	Sx1262_Flag.Irq_Status = GetIrqStatus();
	ClearIrqStatus(TxDone_IRQ);             // 清除发送完成中断标志，防止重复触发
}

/**
  * @brief  初始化SX1262接收模式
  * @param  None
  * @retval None
  */
void Rx_Init(void)
{
	Sx1262_Flag.is_tx = 0;                  // 清除发送状态标志位，置为接收模式
	SetBufferBaseAddress(0,0);              // 设置FIFO收发基地址为0
	SetDioIrqParams(RxDone_IRQ);            // 使能接收完成中断
	SetRx(0);                               // 启动接收模式，永不超时
}

/**
  * @brief  SX1262射频芯片核心参数配置
  * @param  None
  * @retval None
  * @note   统一配置射频所有核心参数，复位后必须调用
  */
void sx1262_Config(void)
{
	SetStandby(0);                          // 进入RC待机模式
	SetRegulatorMode();                     // 配置稳压器模式
	SetPaConfig();                          // 配置功率放大器
	SetDIO2AsRfSwitchCtrl();                // 配置DIO2为射频开关控制
	
	SetPacketType(Sx1262_Flag.RFCtrl.RFmode);	    // 设置调制模式 LoRa/GFSK
	SetRfFrequency(470000000);		            // 设置射频频率 470MHz
	SetTxParams(22,SET_RAMP_1700U);	            // 设置发射功率22dBm，爬升时间1700us
	SetModulationParams(Sx1262_Flag.RFCtrl.SpreadingFactor, Sx1262_Flag.RFCtrl.Bandwidth, Sx1262_Flag.RFCtrl.CodeRate, Sx1262_Flag.RFCtrl.RFmode);

	SetPacketParams(Sx1262_Flag.payload_length);    // 配置数据包参数
}

/**
  * @brief  SX1262射频芯片初始化入口函数
  * @param  None
  * @retval None
  * @note   系统上电后调用一次，完成芯片复位+参数配置+进入接收模式
  */
void RFInit( void )
{
	// 初始化LoRa核心参数
	Sx1262_Flag.RFCtrl.RFmode = LDRO_ON;        // 开启LoRa模式+低速优化
	Sx1262_Flag.RFCtrl.Bandwidth = LORA_BW_125; // LoRa带宽 125KHz
	Sx1262_Flag.RFCtrl.CodeRate = LORA_CR_4_5;  // 编码率 4/5
	Sx1262_Flag.RFCtrl.SpreadingFactor = SF7;   // 扩频因子 SF7
	reset_sx1262();		// 硬件复位射频芯片
	sx1262_Config();    // 配置射频参数
	RFRxMode();         // 初始化并进入接收模式
}

/**
  * @brief  设置SX1262进入接收模式
  * @param  None
  * @retval None
  */
void RFRxMode( void )
{
   Rx_Init();
}

