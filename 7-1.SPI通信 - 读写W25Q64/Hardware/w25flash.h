/*
 * w25flash.h
 * W25Qxx SPI Flash 头文件（已整理）
 */

#ifndef __FLASH_H
#define __FLASH_H

#include "main.h"
#include "gpio.h"

// 每扇区字节数（4KB）
#define WS_W25Qxx_Sector_Size 4096

// 使用的 SPI 总线句柄（按工程实际定义修改）
#define W25Qxx_hspi hspi3

// 片选控制宏（按实际硬件修改）
#define WS_W25Qxx_CS_RESET HAL_GPIO_WritePin(GPIOG, GPIO_PIN_8, GPIO_PIN_RESET)
#define WS_W25Qxx_CS_SET HAL_GPIO_WritePin(GPIOG, GPIO_PIN_8, GPIO_PIN_SET)

// 芯片类型枚举
typedef enum {
    W25Qxx_Not = 0,
    W25Qxx_16 = 0xEF14,
    W25Qxx_32 = 0xEF15,
    W25Qxx_64 = 0xEF16,
    W25Qxx_128 = 0xEF17,
} WS_W25QxxTypeDef;

// W25Qxx 信息结构
typedef struct {
    WS_W25QxxTypeDef chipType;   /*!< 芯片型号 */
    uint32_t chipSectorNbr;      /*!< 扇区数量 */
    uint32_t chipSectorSize;     /*!< 每扇区大小（字节） */
    uint32_t fatfsSectorNbr;     /*!< 可选：fatfs 使用的扇区数 */
    uint32_t fatfsSectorSize;    /*!< 可选：fatfs 扇区大小 */
} WS_W25Qxx_FlashInfoTypeDef;

// 全局声明（在实现文件中定义）
extern WS_W25QxxTypeDef WS_W25Qxx_Chip_Type;
extern WS_W25Qxx_FlashInfoTypeDef W25QxxInfo;

// 命令寄存器定义
#define WS_W25Qxx_Reg_ID 0x90
#define WS_W25Qxx_Reg_Read 0x05
#define WS_W25Qxx_Reg_Write 0x01

#define WS_W25Qxx_Reg_WriteEnable 0x06
#define WS_W25Qxx_Reg_WriteDisable 0x04
#define WS_W25Qxx_Reg_ReadData 0x03
#define WS_W25Qxx_Reg_PageProgram 0x02

#define WS_W25Qxx_Reg_Erase_Sector 0x20
#define WS_W25Qxx_Reg_Erase_Block 0xD8
#define WS_W25Qxx_Reg_Erase_Chip 0xC7

#define WS_W25Qxx_Reg_PowerDown 0xB9
#define WS_W25Qxx_Reg_PowerUp 0xAB
#define WS_W25Qxx_Reg_DeviceID 0xAB

// 接口声明
WS_W25QxxTypeDef WS_W25Qxx_Init(void);

HAL_StatusTypeDef WS_W25Qxx_Write_Page(uint8_t *buffer, uint32_t addr, uint16_t len);
HAL_StatusTypeDef WS_W25Qxx_Read(uint8_t *buffer, uint32_t addr, uint32_t len);
HAL_StatusTypeDef WS_W25Qxx_Write(uint8_t *buffer, uint32_t addr, uint32_t len);

HAL_StatusTypeDef WS_W25Qxx_Sector_Write(uint8_t *buffer, uint32_t sector, uint32_t size);
HAL_StatusTypeDef WS_W25Qxx_Sector_Erase(uint32_t sector);
HAL_StatusTypeDef WS_W25Qxx_Chip_Erase(void);
HAL_StatusTypeDef WS_W25Qxx_Sector_Read(uint8_t *buffer, uint32_t sector, uint32_t size);

uint16_t WS_W25Qxx_Read_ID(void);
HAL_StatusTypeDef WS_W25Qxx_Wait_Idle(void);
uint8_t WS_W25Qxx_Read_SR(void);
void WS_W25Qxx_Write_SR(uint8_t sr);


#endif /* __FLASH_H */

