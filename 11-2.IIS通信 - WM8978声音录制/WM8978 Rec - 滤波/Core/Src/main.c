/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "i2s.h"
#include "sdio.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "wm8978.h"
#include "record.h"
#include "stdio.h"
#include "wav.h"
#include "string.h"
#include "math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
void sd_mount(void)
{
	retSD = f_mount(&SDFatFS, "0:", 1);                                   // 在SD卡上挂载文件系统; 参数：文件系统对象、驱动器路径、读写模式(0只读、1读写)
    if (retSD == FR_NO_FILESYSTEM)                                        // 检查是否已有文件系统，如果没有，就格式化创建创建文件系统
    {
        printf("SD NO FATFS,Initing\r\n");
        static uint8_t aMountBuffer[4096];                                // 格式化时所需的临时缓存; 块大小512的倍数; 值越大格式化越快, 如果内存不够，可改为512或者1024; 当需要在函数内定义这种大缓存时，要用static修饰，令缓存存放在全局数据区内，不然，可能会导致stack溢出。
        retSD = f_mkfs("0:", FM_FAT32, 0, aMountBuffer, sizeof(aMountBuffer));   // 格式化SD卡; 参数：驱动器、文件系统(0-自动\1-FAT12\2-FAT16\3-FAT32\4-exFat)、簇大小(0为自动选择)、格式化临时缓冲区、缓冲区大小; 格式化前必须先f_mount(x,x,1)挂载，即必须用读写方式挂载; 如果SD卡已格式化，f_mkfs()的第2个参数，不能用0自动，必须指定某个文件系统。
        if (retSD == FR_OK)                                               // 格式化 成功
        {
            printf("SD Init : SUCCESS \r\n");
            retSD = f_mount(NULL, "0:", 1);                               // 格式化后，先取消挂载
            retSD = f_mount(&SDFatFS, "0:", 1);                           // 重新挂载
            if (retSD == FR_OK)
                printf("FatFs success \r\n");                            // 挂载成功
            else
                return;                                                   // 挂载失败，退出函数
        }
        else
        {
            printf("SD Init：Fail \r\n");                              // 格式化 失败
            return;
        }
    }
    else if (retSD != FR_OK)                                              // 挂载异常
    {
        printf("FatFs ERROR: %d; Check MX_SDIO_SD_Init() change to 1B\r", retSD);
        return;
    }
    else                                                                  // 挂载成功
    {
        if (SDFatFS.fs_type == 0x03)                                      // FAT32; 1-FAT12、2-FAT16、3-FAT32、4-exFat
            printf("SD FATFS :FAT32\n");
        if (SDFatFS.fs_type == 0x04)                                      // exFAT; 1-FAT12、2-FAT16、3-FAT32、4-exFat
            printf("SD FATFS :exFAT\n");                         
        printf("FatFs success \r\n");                                    // 挂载成功
    }
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define WAV_BUFFER_SIZE 10000
int16_t TempBuf[WAV_BUFFER_SIZE];
#define FS 48000.0f
#define FC_LOW 300.0f    // 带通滤波器低频截止（去除低于300Hz的噪声）
#define FC_HIGH 3400.0f  // 带通滤波器高频截止（去除高于3400Hz的噪声）
#define GATE_THRESHOLD 20  // 门限降噪阈值
int16_t Filter_Buf[WAV_BUFFER_SIZE]={0};  // 滤波后数据（有符号格式）
FIL FilterFile;
uint32_t filter_wavsize=0;
int16_t Tx_Buffer[WAV_BUFFER_SIZE]={0};  // 改为有符号int16_t
int16_t Rx_Buffer[WAV_BUFFER_SIZE]={0};  // 改为有符号int16_t
int16_t Rx_Buffer2[WAV_BUFFER_SIZE]={0}; // 改为有符号int16_t
uint8_t I2S_Callback_Flag=0;
uint8_t use_id=0;
uint8_t mode=0;
char buf[20];  // 用于格式化输出的缓冲区
uint32_t i;    // 循环变量

uint32_t wavsize=0;		// 记录数据长度

/*                KEY   相关函数    begin                       */
#define     Key2         HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_3)
#define     KEY_PORT    (Key2|0xfe)

uint8_t Trg=0;
uint8_t Cont=0,key_temp;
uint8_t ReadData;
void Key_Read(void)	//获取按键状态
{
    ReadData = KEY_PORT^0xff;
    Trg = ReadData & (ReadData ^ Cont);
    Cont = ReadData;
}

uint32_t key_tick=0;	


void key_proc(void){// 按键处理函数
	if(uwTick-key_tick<20) return;	// 消抖
	key_tick=uwTick;
	Key_Read();
	if(Trg)key_temp=Trg;//记录按键最初始的状态
  if(Trg==0 && Cont==0 && key_temp)//所有的按键都弹起
  {
	  if(key_temp == 0x01&&mode==0)
		{ 
				// 按键二按下，并且处于录音状态时
				// 结束录音，配置WAV文件格式
				__HAL_I2S_DISABLE(&hi2s2);
				HAL_NVIC_DisableIRQ(SPI2_IRQn);
				//1.wav
				__WaveHeader *wavhead=(__WaveHeader*)malloc(sizeof(__WaveHeader));
				recoder_wav_init(wavhead); 	
				mode=0x01;	// 切换为暂停模式
				wavhead->riff.ChunkSize=wavsize+36;
				wavhead->data.ChunkSize=wavsize;
				f_lseek(&SDFile,0);	
				UINT br;
				f_write(&SDFile,(const void*)wavhead,sizeof(__WaveHeader),&br);
				f_close(&SDFile);	
				//filter.wav
				__WaveHeader *filter_wavhead=(__WaveHeader*)malloc(sizeof(__WaveHeader));
				recoder_wav_init(filter_wavhead);
				filter_wavhead->riff.ChunkSize = filter_wavsize + 36;
				filter_wavhead->data.ChunkSize = filter_wavsize;
				f_lseek(&FilterFile,0);
				f_write(&FilterFile, filter_wavhead, sizeof(__WaveHeader), &br);
				f_close(&FilterFile);
				free(filter_wavhead);
				free(wavhead);
				f_mount(NULL, "0:", 1); 	
				printf("REC OK:1.wav + filter.wav saved\r\n");
				wavsize=0;
				filter_wavsize=0;
	  }
		  key_temp=0;
	}
	return;
}

/*                  KEY   相关函数   end                       */

// IIR滤波 - 使用4阶Butterworth低通滤波器（级联两个2阶）
typedef struct{float b0,b1,b2,a1,a2;float x1,x2,y1,y2;}IIR_2nd;

// 左右声道各用一组滤波器
IIR_2nd iir_left1, iir_left2;
IIR_2nd iir_right1, iir_right2;

void IIR_2nd_Init(IIR_2nd *filt, float b0, float b1, float b2, float a1, float a2)
{
    filt->b0 = b0;
    filt->b1 = b1;
    filt->b2 = b2;
    filt->a1 = a1;
    filt->a2 = a2;
    filt->x1 = 0;
    filt->x2 = 0;
    filt->y1 = 0;
    filt->y2 = 0;
}

int16_t IIR_2nd_Filter_Sample(IIR_2nd *filt, int16_t in)
{
    float y = filt->b0 * in + filt->b1 * filt->x1 + filt->b2 * filt->x2 
            - filt->a1 * filt->y1 - filt->a2 * filt->y2;
    filt->x2 = filt->x1;
    filt->x1 = in;
    filt->y2 = filt->y1;
    filt->y1 = y;
    if(y > 32767) y = 32767;
    if(y < -32768) y = -32768;
    return (int16_t)y;
}

// 初始化4阶Butterworth带通滤波器（级联2阶高通+2阶低通）
void IIR_LP_Init(float fs, float fc_low, float fc_high)
{
    // 高通滤波器系数（低频截止）
    float omg_low = 2.0f * 3.14159265f * fc_low / fs;
    float k_low = tanf(omg_low / 2.0f);
    float k2_low = k_low * k_low;
    float sqrt2 = 1.41421356f;
    float scale_low = k2_low + sqrt2 * k_low + 1.0f;
    
    float hp_b0 = 1.0f / scale_low;
    float hp_b1 = -2.0f / scale_low;
    float hp_b2 = 1.0f / scale_low;
    float hp_a1 = 2.0f * (k2_low - 1.0f) / scale_low;
    float hp_a2 = (k2_low - sqrt2 * k_low + 1.0f) / scale_low;
    
    // 低通滤波器系数（高频截止）
    float omg_high = 2.0f * 3.14159265f * fc_high / fs;
    float k_high = tanf(omg_high / 2.0f);
    float k2_high = k_high * k_high;
    float scale_high = k2_high + sqrt2 * k_high + 1.0f;
    
    float lp_b0 = k2_high / scale_high;
    float lp_b1 = 2.0f * k2_high / scale_high;
    float lp_b2 = k2_high / scale_high;
    float lp_a1 = 2.0f * (k2_high - 1.0f) / scale_high;
    float lp_a2 = (k2_high - sqrt2 * k_high + 1.0f) / scale_high;
    
    // 级联：先高通滤波，再低通滤波
    IIR_2nd_Init(&iir_left1, hp_b0, hp_b1, hp_b2, hp_a1, hp_a2);   // 左声道高通
    IIR_2nd_Init(&iir_left2, lp_b0, lp_b1, lp_b2, lp_a1, lp_a2);   // 左声道低通
    IIR_2nd_Init(&iir_right1, hp_b0, hp_b1, hp_b2, hp_a1, hp_a2);  // 右声道高通
    IIR_2nd_Init(&iir_right2, lp_b0, lp_b1, lp_b2, lp_a1, lp_a2);  // 右声道低通
}

// 门限降噪
int16_t noise_gate(int16_t sample, int16_t threshold)
{
    if (sample > -threshold && sample < threshold) {
        return 0;
    }
    return sample;
}

// 直接处理有符号音频数据（int16_t格式）
void Frame_Filter(int16_t *in, int16_t *out, uint32_t len)
{
    uint32_t i;
    int32_t sample_l, sample_r;
    
    for(i = 0; i < len/2; i++)
    {
        sample_l = (int32_t)in[2*i];
        sample_r = (int32_t)in[2*i + 1];
        
        // 4阶低通滤波（级联两个2阶）
        sample_l = IIR_2nd_Filter_Sample(&iir_left1, (int16_t)sample_l);
        sample_l = IIR_2nd_Filter_Sample(&iir_left2, (int16_t)sample_l);
        
        sample_r = IIR_2nd_Filter_Sample(&iir_right1, (int16_t)sample_r);
        sample_r = IIR_2nd_Filter_Sample(&iir_right2, (int16_t)sample_r);
        
        // 门限降噪
        sample_l = noise_gate((int16_t)sample_l, GATE_THRESHOLD);
        sample_r = noise_gate((int16_t)sample_r, GATE_THRESHOLD);
        
        // 防溢出处理
        if(sample_l > 32767) sample_l = 32767;
        if(sample_l < -32768) sample_l = -32768;
        if(sample_r > 32767) sample_r = 32767;
        if(sample_r < -32768) sample_r = -32768;
        
        out[2*i] = (int16_t)sample_l;
        out[2*i + 1] = (int16_t)sample_r;
    }
}

// 初始化滤波器状态为静音状态（32768）
void IIR_LP_Init_With_State(float fs, float fc)
{
    float omg = 2.0f * 3.14159265f * fc / fs;
    float k = tanf(omg / 2.0f);
    float k2 = k * k;
    float sqrt2 = 1.41421356f;
    float scale = k2 + sqrt2 * k + 1.0f;
    
    // 初始化左右声道滤波器，状态设为0（对应静音状态）
    IIR_2nd_Init(&iir_left1, k2/scale, 2*k2/scale, k2/scale, 2*(k2-1)/scale, (k2 - sqrt2*k + 1)/scale);
    IIR_2nd_Init(&iir_left2, k2/scale, 2*k2/scale, k2/scale, 2*(k2-1)/scale, (k2 - sqrt2*k + 1)/scale);
    IIR_2nd_Init(&iir_right1, k2/scale, 2*k2/scale, k2/scale, 2*(k2-1)/scale, (k2 - sqrt2*k + 1)/scale);
    IIR_2nd_Init(&iir_right2, k2/scale, 2*k2/scale, k2/scale, 2*(k2-1)/scale, (k2 - sqrt2*k + 1)/scale);
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  SCB->CPACR |= ((3UL << 20)|(3UL << 22));
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  MX_I2S2_Init();
  /* USER CODE BEGIN 2 */
  if(WM8978_Init()) printf("error\r\n");	//WM8978通过I2C配置情况
  else printf("success\r\n");
  WM8978_SPKvol_Set(40);		// 设置喇叭声音
  WM8978_HPvol_Set(40,40);		// 设置耳机音量

  Record_Rec_Init();			// 接收I2C配置寄存器
  IIR_LP_Init(FS, FC_LOW, FC_HIGH);  // 带通滤波器初始化
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

	sd_mount(); // 挂载SD卡
	retSD = f_open(&SDFile, "0:/1.wav", FA_CREATE_ALWAYS | FA_WRITE);	// 每次覆盖录入
	if (retSD != FR_OK){	// 文件打开失败
		printf("open error code: %d",retSD);
		while(1){
			printf(".");
			HAL_Delay(1000);
		}
	}
	retSD = f_open(&FilterFile, "0:/filter.wav", FA_CREATE_ALWAYS | FA_WRITE);  //滤波后的wav
  if(retSD != FR_OK){printf("filter.wav open err:%d\r\n",retSD);while(1);}
	
	
	HAL_I2SEx_TransmitReceive_DMA(&hi2s2, (uint16_t*)Tx_Buffer, (uint16_t*)Rx_Buffer, WAV_BUFFER_SIZE);	// 开启接收
	UINT bw;
	while (1)
	{
		key_proc();	// 按键按下第二个按钮结束录制
		if(I2S_Callback_Flag&&!mode)
		{
			I2S_Callback_Flag=0;
			if(use_id)
			{
				// 写入原始数据WAV文件（二进制格式）
				f_write(&SDFile, Rx_Buffer, WAV_BUFFER_SIZE*2, &bw);
				Frame_Filter(Rx_Buffer,Filter_Buf,WAV_BUFFER_SIZE);//滤波
				// 写入滤波后数据WAV文件（二进制格式）
				f_write(&FilterFile, Filter_Buf, WAV_BUFFER_SIZE*2, &bw);
			}
			else
			{
			  // 写入原始数据WAV文件（二进制格式）
				f_write(&SDFile, Rx_Buffer2, WAV_BUFFER_SIZE*2, &bw);
				Frame_Filter(Rx_Buffer2,Filter_Buf,WAV_BUFFER_SIZE);//滤波
				// 写入滤波后数据WAV文件（二进制格式）
				f_write(&FilterFile, Filter_Buf, WAV_BUFFER_SIZE*2, &bw);
			}
			wavsize+=WAV_BUFFER_SIZE*2;
			filter_wavsize+=WAV_BUFFER_SIZE*2;//新增计数
		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef *hi2s)
{
	if(mode!=0xff){
		I2S_Callback_Flag=1;
		use_id=!use_id;
		if(use_id){	//Rx_Buffer数据采集完毕
			HAL_I2SEx_TransmitReceive_DMA(&hi2s2, (uint16_t*)Tx_Buffer, (uint16_t*)Rx_Buffer2, WAV_BUFFER_SIZE);
		}else{		//Rx_Buffer2数据采集完毕
			HAL_I2SEx_TransmitReceive_DMA(&hi2s2, (uint16_t*)Tx_Buffer, (uint16_t*)Rx_Buffer, WAV_BUFFER_SIZE);
		}
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
