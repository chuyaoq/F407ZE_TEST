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
uint16_t Tx_Buffer[WAV_BUFFER_SIZE]={0};
uint16_t Rx_Buffer[WAV_BUFFER_SIZE]={0};
uint16_t Rx_Buffer2[WAV_BUFFER_SIZE]={0};
uint8_t I2S_Callback_Flag=0;
uint8_t use_id=0;
uint8_t mode=0;

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
    if(Trg==0 && Cont==0 && key_temp)//所有的按键都弹起,但是应该是弹起之后的第一次
    {
		if(key_temp == 0x01&&mode==0){ // 按键二按下，并且处于录音状态时
			// 结束录音，配置WAV文件格式
			__HAL_I2S_DISABLE(&hi2s2);
			HAL_NVIC_DisableIRQ(SPI2_IRQn);
			__WaveHeader *wavhead=(__WaveHeader*)malloc(sizeof(__WaveHeader));
			recoder_wav_init(wavhead); 	// 将数据填充wavhead
			mode=0x01;	// 切换为暂停模式
			wavhead->riff.ChunkSize=wavsize+36;
			wavhead->data.ChunkSize=wavsize;
			f_lseek(&SDFile,0);	// 将文件对象的当前读写位置设置为文件头部
			UINT br;
			f_write(&SDFile,(const void*)wavhead,sizeof(__WaveHeader),&br);// 写入
			f_close(&SDFile);	// 关闭文件
			f_mount(NULL, "0:", 1); 	// 取消挂载
			printf("REC OK\r\n");
			wavsize=0;
		}
		key_temp=0;
	}
	return;
}

/*                  KEY   相关函数   end                       */
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
	HAL_I2SEx_TransmitReceive_DMA(&hi2s2,Tx_Buffer,Rx_Buffer, WAV_BUFFER_SIZE);	// 开启接收
	UINT bw;
	while (1)
	{
		key_proc();	// 按键按下第二个按钮结束录制
		if(I2S_Callback_Flag&&!mode){
			I2S_Callback_Flag=0;
			if(use_id){	// 1：Rx_Buffer采集完毕
				f_write(&SDFile,Rx_Buffer,WAV_BUFFER_SIZE*2,&bw);// 写入SD卡
			}else{	// 0：RxBuffer2采集完毕
				f_write(&SDFile,Rx_Buffer2,WAV_BUFFER_SIZE*2,&bw);// 写入SD卡
			}
			wavsize+=WAV_BUFFER_SIZE*2;// 总长度文件写入u8，I2S采集u16，故总长度+WAV_BUFFER_SIZE*2
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
			HAL_I2SEx_TransmitReceive_DMA(&hi2s2,Tx_Buffer,Rx_Buffer2, WAV_BUFFER_SIZE);
		}else{		//Rx_Buffer2数据采集完毕
			HAL_I2SEx_TransmitReceive_DMA(&hi2s2,Tx_Buffer,Rx_Buffer, WAV_BUFFER_SIZE);
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
