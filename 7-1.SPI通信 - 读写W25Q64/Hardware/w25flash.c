#include "w25flash.h" // 包含 W25Qxx 驱动相关声明
#include "string.h" // 包含 memcpy/memset/memcmp 等内存函数
#include "stdio.h" // 包含 printf 等标准 IO 函数


extern SPI_HandleTypeDef W25Qxx_hspi; // 外部定义的 SPI 句柄，用于与 Flash 通信

WS_W25QxxTypeDef WS_W25Qxx_Chip_Type = W25Qxx_Not; // 当前识别到的芯片类型（初始为未识别）

WS_W25Qxx_FlashInfoTypeDef W25QxxInfo; // 存放芯片信息的结构体实例

uint8_t W25QxxWriteBuffer[4096]; // 临时缓冲区，用于整扇区读写（4KB）


// 函数: WS_W25Qxx_Read_Byte
// 描述: 通过 SPI 接收一个字节并返回
uint8_t WS_W25Qxx_Read_Byte(void) // 从 SPI 接收 1 字节
{
	uint8_t r_date; // 接收数据缓冲
	HAL_SPI_Receive(&W25Qxx_hspi, &r_date, 1, 100); // 接收 1 字节，超时 100ms
	return r_date; // 返回接收到的字节
}


// 函数: WS_W25Qxx_Write_Byte
// 描述: 通过 SPI 发送一个字节
void WS_W25Qxx_Write_Byte(uint8_t t_date) // 向 SPI 发送 1 字节
{
	HAL_SPI_Transmit(&W25Qxx_hspi, &t_date, 1, 100); // 发送 1 字节，超时 100ms
}


// 函数: WS_W25Qxx_Read_Buffer
// 描述: 通过 SPI 接收多个字节到缓冲区
// 返回: HAL 状态
HAL_StatusTypeDef WS_W25Qxx_Read_Buffer(uint8_t *buffer, uint16_t len) // 接收 len 字节到 buffer
{
	return HAL_SPI_Receive(&W25Qxx_hspi, buffer, len, 100); // 返回 HAL 状态
}


// 函数: WS_W25Qxx_Write_Buffer
// 描述: 通过 SPI 发送缓冲区数据
// 返回: HAL 状态
HAL_StatusTypeDef WS_W25Qxx_Write_Buffer(uint8_t *buffer, uint16_t len) // 发送 len 字节来自 buffer
{
	return HAL_SPI_Transmit(&W25Qxx_hspi, buffer, len, 100); // 返回 HAL 状态
}



// 函数: WS_W25Qxx_Init
// 描述: 初始化 W25Qxx 芯片并读取芯片 ID，填充芯片信息结构
// 返回: 芯片类型
WS_W25QxxTypeDef WS_W25Qxx_Init(void) // 初始化并识别芯片
{
	if (WS_W25Qxx_Chip_Type == W25Qxx_Not) // 仅在尚未识别时执行
	{
		uint16_t id = WS_W25Qxx_Read_ID(); // 读取芯片 ID（尝试一次）
		id = WS_W25Qxx_Read_ID(); // 再次读取以提高可靠性

		memset((char *)&W25QxxInfo, 0, sizeof(WS_W25Qxx_FlashInfoTypeDef)); // 清零信息结构体
		switch (id) // 根据 ID 设置芯片类型及扇区数
		{
		case W25Qxx_16: // 16M
			WS_W25Qxx_Chip_Type = W25Qxx_16; // 设置类型
			W25QxxInfo.chipSectorNbr = 512; // 扇区数（4KB 扇区）
			break;

		case W25Qxx_32: // 32M
			WS_W25Qxx_Chip_Type = W25Qxx_32; // 设置类型
			W25QxxInfo.chipSectorNbr = 1024; // 扇区数
			break;

		case W25Qxx_64: // 64M
			WS_W25Qxx_Chip_Type = W25Qxx_64; // 设置类型
			W25QxxInfo.chipSectorNbr = 2048; // 扇区数
			break;

		case W25Qxx_128: // 128M
			WS_W25Qxx_Chip_Type = W25Qxx_128; // 设置类型
			W25QxxInfo.chipSectorNbr = 4096; // 扇区数
			break;
		}
		if (WS_W25Qxx_Chip_Type != W25Qxx_Not) // 若识别成功
		{
			W25QxxInfo.chipType = WS_W25Qxx_Chip_Type; // 填充芯片类型字段
			W25QxxInfo.chipSectorSize = WS_W25Qxx_Sector_Size; // 填充扇区大小字段
			printf("W25Qxx id : %X size %d M \r\n", id, W25QxxInfo.chipSectorNbr * W25QxxInfo.chipSectorSize / 1024 / 1024); // 打印信息
		}
		else // 识别失败
		{
			printf("Not W25Qxx %X\r\n", id); // 打印未识别信息
		}
	}
	return WS_W25Qxx_Chip_Type; // 返回识别结果
}


// 函数: WS_W25Qxx_Read_ID
// 描述: 读取芯片 ID（发送 ID 命令并返回 16-bit ID）
// 返回: 芯片 ID
uint16_t WS_W25Qxx_Read_ID(void) // 读取并返回 16 位芯片 ID
{
	uint16_t id = 0; // 存放 ID
	WS_W25Qxx_CS_RESET; HAL_Delay(50); // 片选拉低，开始事务
	WS_W25Qxx_Write_Byte(WS_W25Qxx_Reg_ID); // 发送读取 ID 命令
	WS_W25Qxx_Write_Byte(0x00); // 发送占位字节
	WS_W25Qxx_Write_Byte(0x00); // 发送占位字节
	WS_W25Qxx_Write_Byte(0x00); // 发送占位字节
	id |= WS_W25Qxx_Read_Byte() << 8; // 读取高字节
	id |= WS_W25Qxx_Read_Byte(); // 读取低字节
	WS_W25Qxx_CS_SET; // 释放片选，结束事务
	return id; // 返回 ID
}


// 函数: WS_W25Qxx_Read_SR
// 描述: 读取状态寄存器，返回状态字节
// 返回: 状态寄存器字节
uint8_t WS_W25Qxx_Read_SR(void) // 读取状态寄存器并返回
{
	uint8_t byte = 0; // 存放状态字节
	WS_W25Qxx_CS_RESET; HAL_Delay(50); // 开始事务
	WS_W25Qxx_Write_Byte(WS_W25Qxx_Reg_Read); // 发送读取状态寄存器命令
	byte = WS_W25Qxx_Read_Byte(); // 读取状态
	WS_W25Qxx_CS_SET; // 结束事务
	return byte; // 返回状态字节
}


// 函数: WS_W25Qxx_Write_SR
// 描述: 写状态寄存器（注意仅部分位可写）
// 参数: date 要写入的状态寄存器字节
void WS_W25Qxx_Write_SR(uint8_t date) // 写状态寄存器
{
	WS_W25Qxx_CS_RESET; HAL_Delay(50); // 开始事务
	WS_W25Qxx_Write_Byte(WS_W25Qxx_Reg_Write); // 发送写状态寄存器命令
	WS_W25Qxx_Write_Byte(date); // 发送要写入的字节
	WS_W25Qxx_CS_SET; // 结束事务
}


// 函数: WS_W25Qxx_Write_Enable
// 描述: 使能写操作（设置写使能位）
void WS_W25Qxx_Write_Enable(void) // 写使能
{
	WS_W25Qxx_CS_RESET; HAL_Delay(50); // 开始事务
	WS_W25Qxx_Write_Byte(WS_W25Qxx_Reg_WriteEnable); // 发送写使能命令
	WS_W25Qxx_CS_SET; // 结束事务
}


// 函数: WS_W25Qxx_Write_Disable
// 描述: 禁用写操作（清除写使能位）
void WS_W25Qxx_Write_Disable(void) // 写禁止
{
	WS_W25Qxx_CS_RESET; HAL_Delay(50); // 开始事务
	HAL_Delay(50);
	WS_W25Qxx_Write_Byte(WS_W25Qxx_Reg_WriteDisable); // 发送写禁止命令
	HAL_Delay(50);
	WS_W25Qxx_CS_SET; // 结束事务
}


// 函数: WS_W25Qxx_Read
// 描述: 从指定地址读取指定长度的数据到 buffer
// 返回: HAL 状态
HAL_StatusTypeDef WS_W25Qxx_Read(uint8_t *buffer, uint32_t addr, uint32_t len) // 从地址读取 len 字节
{
	HAL_StatusTypeDef rtn; // 存放 HAL 返回状态
	WS_W25Qxx_CS_RESET; HAL_Delay(50); // 开始事务
	WS_W25Qxx_Write_Byte(WS_W25Qxx_Reg_ReadData); // 发送读命令
	WS_W25Qxx_Write_Byte((uint8_t)((addr) >> 16)); // 地址高字节
	WS_W25Qxx_Write_Byte((uint8_t)((addr) >> 8)); // 地址中间字节
	WS_W25Qxx_Write_Byte((uint8_t)addr); // 地址低字节
	rtn = WS_W25Qxx_Read_Buffer(buffer, len); // 接收数据到 buffer
	WS_W25Qxx_CS_SET; // 结束事务
	return rtn; // 返回 HAL 状态
}


// 函数: WS_W25Qxx_Write_Page
// 描述: 在页面内写入数据（最多 256 字节）
// 返回: 等待操作完成的 HAL 状态
HAL_StatusTypeDef WS_W25Qxx_Write_Page(uint8_t *buffer, uint32_t addr, uint16_t len) // 单页写（len<=256）
{
	WS_W25Qxx_Write_Enable(); // 写使能
	WS_W25Qxx_CS_RESET; HAL_Delay(50); // 开始事务
	WS_W25Qxx_Write_Byte(WS_W25Qxx_Reg_PageProgram); // 页面编程命令
	WS_W25Qxx_Write_Byte((uint8_t)((addr) >> 16)); // 地址高字节
	WS_W25Qxx_Write_Byte((uint8_t)((addr) >> 8)); // 地址中间字节
	WS_W25Qxx_Write_Byte((uint8_t)addr); // 地址低字节
	WS_W25Qxx_Write_Buffer(buffer, len); // 发送页面数据
	WS_W25Qxx_CS_SET; // 结束事务
	return WS_W25Qxx_Wait_Idle(); // 等待写完成
}


// 函数: WS_W25Qxx_Write
// 描述: 按地址写入任意长度数据（会处理扇区擦除、页编程）
// 返回: HAL 状态
HAL_StatusTypeDef WS_W25Qxx_Write(uint8_t *buffer, uint32_t addr, uint32_t len) // 按扇区写入任意长度
{
	uint32_t size; // 页面偏移临时变量
	uint32_t chipSector, addrAdd, writeCount, writeAddr; // 扇区号、扇区内偏移、写入长度、扇区起始地址
	uint16_t i = 0, l; // 循环与临时变量

	if (len == 0 || buffer == NULL || addr > W25QxxInfo.chipSectorNbr * W25QxxInfo.chipSectorSize) // 参数检查
		return HAL_ERROR; // 参数非法时返回错误

	while (len > 0) // 循环写入直至写完
	{
		l = addr % WS_W25Qxx_Sector_Size; // 计算在扇区内的偏移
		if ((WS_W25Qxx_Sector_Size - l) < len) // 若当前扇区剩余空间不足以写入全部数据
		{
			writeCount = (WS_W25Qxx_Sector_Size - l); // 本次写入扇区剩余空间
			len -= (WS_W25Qxx_Sector_Size - l); // 减去已写长度
		}
		else
		{
			writeCount = len; // 本次写入全部剩余数据
			len -= len; // 将 len 置零
		}

		chipSector = addr / 4096; // 计算扇区号（以 4KB 为单位）
		addrAdd = addr % 4096; // 扇区内偏移
		writeAddr = chipSector * 4096; // 扇区起始地址

		WS_W25Qxx_Read(W25QxxWriteBuffer, chipSector * (WS_W25Qxx_Sector_Size), WS_W25Qxx_Sector_Size); // 读取整个扇区到临时缓冲

		memcpy(W25QxxWriteBuffer + addrAdd, buffer, writeCount); // 在缓冲区中合并要写入的数据

		WS_W25Qxx_Sector_Erase(chipSector); // 擦除扇区

		for (i = 0; i < 16; i++) // 扇区由 16 个 256B 页面组成，逐页写入
		{
			size = (i * 256); // 计算页面偏移
			WS_W25Qxx_Write_Page(W25QxxWriteBuffer + size, writeAddr + size, 256); // 写入页面
		}
		WS_W25Qxx_Read(W25QxxWriteBuffer, addr, writeCount); // 读取写回的数据进行校验

		if (memcmp(W25QxxWriteBuffer, buffer, writeCount) != 0) // 校验写入是否成功
		{
			return HAL_ERROR; // 校验失败则返回错误
		}

		addr += writeCount; // 更新写地址
		buffer += writeCount; // 移动源指针
	}
	return HAL_OK; // 写入成功
}


// 函数: WS_W25Qxx_Sector_Erase
// 描述: 擦除指定扇区（4096 字节）
// 返回: 等待擦除完成的 HAL 状态
HAL_StatusTypeDef WS_W25Qxx_Sector_Erase(uint32_t sector) // 擦除指定扇区
{
	uint32_t addr = WS_W25Qxx_Sector_Size * sector; // 计算扇区起始地址
	WS_W25Qxx_Write_Enable(); // 写使能
	WS_W25Qxx_Wait_Idle(); // 确保设备空闲
	WS_W25Qxx_CS_RESET; HAL_Delay(50); // 开始事务
	WS_W25Qxx_Write_Byte(WS_W25Qxx_Reg_Erase_Sector); // 发送扇区擦除命令
	WS_W25Qxx_Write_Byte((uint8_t)((addr) >> 16)); // 地址高字节
	WS_W25Qxx_Write_Byte((uint8_t)((addr) >> 8)); // 地址中间字节
	WS_W25Qxx_Write_Byte((uint8_t)addr); // 地址低字节
	WS_W25Qxx_CS_SET; // 结束事务
	return WS_W25Qxx_Wait_Idle(); // 等待擦除完成并返回状态
}


// 备注: 写入扇区函数，按扇区为单位写入若干个扇区
HAL_StatusTypeDef WS_W25Qxx_Sector_Write(uint8_t *buffer, uint32_t sector, uint32_t len) // 按扇区号写入多个扇区
{
	return WS_W25Qxx_Write(buffer, sector * WS_W25Qxx_Sector_Size, len * WS_W25Qxx_Sector_Size); // 转换扇区到地址并写入
}


// 备注: 读取扇区数据（按扇区数量读取）
HAL_StatusTypeDef WS_W25Qxx_Sector_Read(uint8_t *buffer, uint32_t sector, uint32_t len) // 按扇区号读取多个扇区
{
	return WS_W25Qxx_Read(buffer, sector * WS_W25Qxx_Sector_Size, len * WS_W25Qxx_Sector_Size); // 转换并读取
}


// 函数: WS_W25Qxx_Wait_Idle
// 描述: 等待芯片退出 BUSY 状态，超时返回 HAL_TIMEOUT
// 返回: HAL_OK 或 HAL_TIMEOUT
HAL_StatusTypeDef WS_W25Qxx_Wait_Idle(void) // 等待芯片空闲，超时返回 HAL_TIMEOUT
{
	int i = 0, n; // i 为超时计数器，n 用于短延时
	while ((WS_W25Qxx_Read_SR() & 0x01) == 0x01) // 读状态寄存器 BUSY 位为 1 则等待
	{
		for (n = 0; n < 8800; n++) {} // 简单空循环延时
		if (++i >= 1000) // 超过最大尝试次数则跳出
			break;
	}
	if (i < 1000)
		return HAL_OK; // 在超时前退出，返回 OK
	else
		return HAL_TIMEOUT; // 超时返回
}


// 函数: WS_W25Qxx_Chip_Erase
// 描述: 擦除整片 FLASH
// 返回: 等待擦除完成的 HAL 状态
HAL_StatusTypeDef WS_W25Qxx_Chip_Erase(void) // 擦除整片 Flash
{
	WS_W25Qxx_Write_Enable(); // 写使能
	WS_W25Qxx_Wait_Idle(); // 等待空闲
	WS_W25Qxx_CS_RESET; HAL_Delay(50); // 开始事务
	WS_W25Qxx_Write_Byte(WS_W25Qxx_Reg_Erase_Chip); // 发送整片擦除命令
	WS_W25Qxx_CS_SET; // 结束事务
	return WS_W25Qxx_Wait_Idle(); // 等待擦除完成
}


// 回调: 从最后一个扇区读取系统配置结构
void WS_Config_Read_Struct_Callback(uint8_t *date, uint32_t len) // 回调：读取最后扇区的配置数据
{
	WS_W25Qxx_Read(date, (W25QxxInfo.chipSectorNbr - 1) * 4096, len); // 从最后一个扇区读取 len 字节
}


// 回调: 将系统配置结构写入最后一个扇区
void WS_Config_Write_Struct_Callback(uint8_t *date, uint32_t len) // 回调：写入最后扇区的配置数据
{
	WS_W25Qxx_Write(date, (W25QxxInfo.chipSectorNbr - 1) * 4096, len); // 写入 len 字节到最后扇区
}
