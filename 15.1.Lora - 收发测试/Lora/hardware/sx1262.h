#ifndef  __SX1262_H__
#define  __SX1262_H__
#include "main.h"


#define  RF_IRQ()           (GPIOG->IDR & LORA1_DIO1_Pin)    //LoRa中断引脚

/**********************************************/
#define  SF5   0x05
#define  SF6   0x06 
#define  SF7   0x07
#define  SF8   0x08
#define  SF9   0x09
#define  SF10  0x0A 
#define  SF11  0x0B 
#define  SF12  0x0C 

#define  LORA_BW_7	  0x00
#define  LORA_BW_10	  0x08
#define  LORA_BW_15	  0x01 
#define  LORA_BW_20	  0x09 
#define  LORA_BW_31	  0x02
#define  LORA_BW_41	  0x0A 
#define  LORA_BW_62	  0x03 
#define  LORA_BW_125  0x04 
#define  LORA_BW_250  0x05 
#define  LORA_BW_500  0x06

#define  LORA_CR_4_5  0x01
#define  LORA_CR_4_6  0x02
#define  LORA_CR_4_7  0x03
#define  LORA_CR_4_8  0x04

#define  LDRO_ON	  0x01
#define  LDRO_OFF     0x00
/**********************************************/
#define SET_SLEEP 			   0x84
#define SET_STANDBY			   0x80
#define SET_TX				     0x83
#define SET_RX				     0x82
#define SET_PACKET_TYPE		 0x8A
#define SET_RF_FREQUENCY 	 0x86
#define SET_TX_PARAMS 		 0x8E
#define SET_BUF_BASE_ADDR  0x8F



#define SET_RAMP_10U  	   0x00
#define SET_RAMP_20U  	   0x01
#define SET_RAMP_40U 	     0x02
#define SET_RAMP_80U 	     0x03
#define SET_RAMP_200U 	   0x04
#define SET_RAMP_800U 	   0x05
#define SET_RAMP_1700U 	   0x06
#define SET_RAMP_3400U 	   0x07

#define  TxDone_IRQ  	     0x01
#define  RxDone_IRQ  	     0x02
#define  Pream_IRQ 		     0x04
#define  SyncWord_IRQ 	   0x08
#define  Header_IRQ  	     0x10
#define  HeaderErr_IRQ 	   0x20
#define  CrcErr_IRQ		     0x40
#define  CadDone_IRQ	     0x80
#define  CadDetec_IRQ	     0x0100
#define  Timeout_IRQ	     0x0200

#define  DIO3_1_6V  0x00
#define  DIO3_1_7V  0x01
#define  DIO3_1_8V  0x02
#define  DIO3_2_2V  0x03
#define  DIO3_2_4V  0x04
#define  DIO3_2_7V  0x05
#define  DIO3_3_0V  0x06
#define  DIO3_3_3V  0x07

typedef struct 
{
	uint8_t reach_tx;	
	uint8_t is_tx	;
	uint8_t rf_reach_timeout;
	uint8_t busy_timeout;
	uint8_t busy_timeout_cnt;
	uint8_t tx_buff_flag;
	uint8_t txbuf[128];
	uint8_t payload_length;
	uint8_t rf_timeout;
	uint16_t Irq_Status;
	struct RFCtrl
	{
	  uint8_t  RFmode;    //0:FSK,1:LoRa
	  uint8_t  SpreadingFactor;   //扩频因子,SF5~SF12
		uint8_t  Bandwidth; //带宽LORA_BW_7~LORA_BW_500
		uint8_t  CodeRate;  //编码率
	} RFCtrl;
	int8_t   RxRssi;
}FlagType;
extern FlagType  Sx1262_Flag;

void Sx1262_SPI_Init(void);
void reset_sx1262(void);

void sx1262_Config(void);
void Tx_Start(uint8_t *txbuf,uint8_t payload_length);
void Rx_Init(void);

void timerx_init(void);

void SetPacketType(uint8_t PacketType);
uint8_t GetPacketType(void);
uint16_t GetIrqStatus(void);
void ClearIrqStatus(uint16_t irq);
void ReadBuffer(uint8_t offset, uint8_t *data, uint8_t length);
void GetRxBufferStatus(uint8_t *payload_len, uint8_t *buf_pointer);
void SetStandby(uint8_t StdbyConfig);
uint8_t GetRssiInst(void);
void RFRxMode( void );
void RFInit( void );
#endif

