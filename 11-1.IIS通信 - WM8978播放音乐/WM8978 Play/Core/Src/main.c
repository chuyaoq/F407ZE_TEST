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
#include "audioplay.h"
#include "string.h"
#include "audioplay.h"
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

#define WAV_BUFFER_SIZE 4096*2
uint16_t Tx_Buffer[WAV_BUFFER_SIZE];  //The buffer of record.
uint16_t Tx_Buffer1[WAV_BUFFER_SIZE];  //The buffer of record.
uint8_t I2S_Callback_Flag=0;
_Bool Buf_flag=0;	//0:使用buf 1：使用buf1

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



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
/*  
	PB10---TX
	PB11---RX
*/
  if(WM8978_Init()) printf("error\r\n");	//WM8978通过I2C配置情况
  else printf("success\r\n");
  WM8978_SPKvol_Set(100);		// 设置喇叭声音
  WM8978_HPvol_Set(20,20);		// 设置耳机音量
  Record_Play_Init();			// 播放I2C配置寄存器
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	sd_mount();
	u8 res=Play_music((u8*)"0:/2.wav");
	if(res){// 文件打开失败
		while(1){
			printf(".");
			HAL_Delay(1000);
		}
	}
	wav_buffill((uint8_t*)Tx_Buffer,WAV_BUFFER_SIZE*2,wavctrl.bps);	//填充Tx_Buffer
	wav_buffill((uint8_t*)Tx_Buffer1,WAV_BUFFER_SIZE*2,wavctrl.bps);//填充Tx_Buffer1
	HAL_I2S_Transmit_DMA(&hi2s2,Tx_Buffer,WAV_BUFFER_SIZE);	//播放Tx_Buffer
	while (1)
	{
		if(I2S_Callback_Flag){	//I2S buf播放完成
			I2S_Callback_Flag=0;
			if(Buf_flag==0){	//Tx_Buffer播放完毕，播放Tx_Buffer1，填充Tx_Buffer
				HAL_I2S_Transmit_DMA(&hi2s2,Tx_Buffer1,WAV_BUFFER_SIZE);
				if(!wav_buffill((uint8_t*)Tx_Buffer,WAV_BUFFER_SIZE*2,wavctrl.bps)) f_lseek(&SDFile,wavctrl.datastart);//填充Tx_Buffer，播放完重新播放
			}else{				//Tx_Buffer1播放完毕，播放Tx_Buffer，填充Tx_Buffer1
				HAL_I2S_Transmit_DMA(&hi2s2,Tx_Buffer,WAV_BUFFER_SIZE);
				if(!wav_buffill((uint8_t*)Tx_Buffer1,WAV_BUFFER_SIZE*2,wavctrl.bps)) f_lseek(&SDFile,wavctrl.datastart);//填充Tx_Buffer1，播放完重新播放
			}
			Buf_flag=!Buf_flag;
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
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)	//发送完成
{
	I2S_Callback_Flag=1;
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
