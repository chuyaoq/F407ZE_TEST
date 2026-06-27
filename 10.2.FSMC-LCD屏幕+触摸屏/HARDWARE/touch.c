#include "touch.h"
#include "lcd.h"
#include "stdlib.h"
#include "math.h"
#include "stdio.h"
#include "usart.h"
#include "delay_us.h"
#include "gpio.h"
_m_tp_dev tp_dev=
{
	TP_Init,
	TP_Scan,
	TP_Adjust,
	0,
	0, 
	0,
	0,
	0,
	0,	  	 		
	0,
};					

#if (DFT_SCAN_DIR>>4)%4
uint8_t CMD_RDX=0X90;
uint8_t CMD_RDY=0XD0;
#else
uint8_t CMD_RDX=0XD0;
uint8_t CMD_RDY=0X90;
#endif
 	 			    	

//SPI写数据
//向触摸屏IC写入1byte数据    
//num:要写入的数据
void TP_Write_Byte(uint8_t num)    
{
	uint8_t count=0;   
	for(count=0;count<8;count++)  
	{ 	  
		if(num&0x80)
		{			
			HAL_GPIO_WritePin(TDIN_GPIO_Port, TDIN_Pin,GPIO_PIN_SET);
		}
		else 
		{  
			HAL_GPIO_WritePin(TDIN_GPIO_Port, TDIN_Pin,GPIO_PIN_RESET);   
		}
		num<<=1;    
		HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_RESET);
		delay_us(1);
		HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_SET);		//上升沿有效	        
	}		 			    
} 		 
//SPI读数据 
//从触摸屏IC读取adc值
//CMD:指令
//返回值:读到的数据	   
uint16_t TP_Read_AD(uint8_t CMD)	  
{
	uint8_t count=0; 	  
	uint16_t Num=0; 
	HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_RESET);		//先拉低时钟 	 
	HAL_GPIO_WritePin(TDIN_GPIO_Port, TDIN_Pin,GPIO_PIN_RESET); 	//拉低数据线
	HAL_GPIO_WritePin(TCS_GPIO_Port, TCS_Pin,GPIO_PIN_RESET); 		//选中触摸屏IC
	TP_Write_Byte(CMD);//发送命令字
	delay_us(6);
	HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_RESET); 	     	    
	delay_us(1);    	   
	HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_SET);		//给1个时钟，清除BUSY
	delay_us(1);    
	HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_RESET); 	     	    
	for(count=0;count<16;count++)//读出16位数据,只有高12位有效 
	{ 				  
		Num<<=1; 	 
		HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_RESET);	//下降沿有效  	    	   
		delay_us(1);    
 		HAL_GPIO_WritePin(TCLK_GPIO_Port, TCLK_Pin, GPIO_PIN_SET);
 		if(HAL_GPIO_ReadPin(DOUT_GPIO_Port, DOUT_Pin)==GPIO_PIN_SET)
		{
			Num++;
		}			
	}  	
	Num>>=4;   	//只有高12位有效.
	HAL_GPIO_WritePin(TCS_GPIO_Port, TCS_Pin,GPIO_PIN_SET);		//释放片选	 
	 printf("CMD: 0x%02X, Read: %d\r\n", CMD, Num);
	return(Num);   
}
//读取一个坐标值(x或者y)
//连续读取READ_TIMES次数据,对这些数据升序排列,
//然后去掉最低和最高LOST_VAL个数,取平均值 
//xy:指令（CMD_RDX/CMD_RDY）
//返回值:读到的数据
#define READ_TIMES 5 	//读取次数
#define LOST_VAL 1	  	//丢弃值
uint16_t TP_Read_XOY(uint8_t xy)
{
	uint16_t i, j;
	uint16_t buf[READ_TIMES];
	uint16_t sum=0;
	uint16_t temp;
	for(i=0;i<READ_TIMES;i++)buf[i]=TP_Read_AD(xy);		 		    
	for(i=0;i<READ_TIMES-1; i++)//排序
	{
		for(j=i+1;j<READ_TIMES;j++)
		{
			if(buf[i]>buf[j])//升序排列
			{
				temp=buf[i];
				buf[i]=buf[j];
				buf[j]=temp;
			}
		}
	}	  
	sum=0;
	for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];
	temp=sum/(READ_TIMES-2*LOST_VAL);
	return temp;   
} 
//读取x,y坐标
//最小值不能少于100.
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
uint8_t TP_Read_XY(uint16_t *x,uint16_t *y)
{
	uint16_t xtemp,ytemp;
	xtemp=TP_Read_XOY(CMD_RDX);
	ytemp=TP_Read_XOY(CMD_RDY);
	*x=xtemp;
	*y=ytemp;
	return 1;//读数成功
}
//连续2次读取触摸屏IC,且这两次的偏差不能超过
//ERR_RANGE,满足条件,则认为读数正确,否则读数错误.	   
//该函数能大大提高准确度
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
#define ERR_RANGE 50 //误差范围 
uint8_t TP_Read_XY2(uint16_t *x,uint16_t *y) 
{
	uint16_t x1,y1;
 	uint16_t x2,y2;
 	uint8_t flag;    
	flag=TP_Read_XY(&x1,&y1);   
	if(flag==0)return(0);
	flag=TP_Read_XY(&x2,&y2);	   
	if(flag==0)return(0);
	if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))//前后两次采样在+-50内
	&&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
	{
		*x=(x1+x2)/2;
		*y=(y1+y2)/2;
		return 1;
	}else return 0;	  
}  
//////////////////////////////////////////////////////////////////////////////////		  
//与LCD部分有关的函数  
//画一个触摸点
//用来校准用的
//x,y:坐标
//color:颜色
void TP_Drow_Touch_Point(uint16_t x,uint16_t y,uint16_t color)
{
	LCD_DrawLine(x-12,y,x+13,y,color);//横线
	LCD_DrawLine(x,y-12,x,y+13,color);//竖线
	LCD_DrawPoint(x+1,y+1,color);
	LCD_DrawPoint(x-1,y+1,color);
	LCD_DrawPoint(x+1,y-1,color);
	LCD_DrawPoint(x-1,y-1,color);
	Draw_Circle(x,y,6,color);//画中心圈
}	  
//画一个大点(2*2的点)		   
//x,y:坐标
//color:颜色
void TP_Draw_Big_Point(uint16_t x,uint16_t y,uint16_t color)
{	    
	LCD_DrawPoint(x,y,color);//中心点 
	LCD_DrawPoint(x+1,y,color);
	LCD_DrawPoint(x,y+1,color);
	LCD_DrawPoint(x+1,y+1,color);	 	  	
}

/******************************************************************************
      函数说明：画线
      入口数据：x1,y1   起始坐标
                x2,y2   终止坐标
                color   线的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawRoughLine(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color)
{
	uint16_t t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1;
	uRow=x1;//画线起点坐标
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步方向 
	else if (delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//水平线 
	else {incy=-1;delta_y=-delta_y;}
	if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		TP_Draw_Big_Point(uRow,uCol,color);//画点
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////		  
//触摸按键扫描
//tp:0,屏幕坐标;1,物理坐标(校准等特殊场合用)
//返回值:当前触屏状态.
//0,触屏无触摸;1,触屏有触摸
uint8_t TP_Scan(uint8_t tp)
{			   
	if(HAL_GPIO_ReadPin(PEN_GPIO_Port, PEN_Pin)==RESET)//有按键按下
	{
		if(tp)TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]);//读取物理坐标
		else if(TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]))//读取屏幕坐标
		{
	 		tp_dev.x[0]=tp_dev.xfac*tp_dev.x[0]+tp_dev.xoff;//将结果转换为屏幕坐标
			tp_dev.y[0]=tp_dev.yfac*tp_dev.y[0]+tp_dev.yoff;  
	 	} 
		if((tp_dev.sta&TP_PRES_DOWN)==0)//之前没有被按下
		{		 
			tp_dev.sta=TP_PRES_DOWN|TP_CATH_PRES;//按键按下  
			tp_dev.x[4]=tp_dev.x[0];//记录第一次按下时的坐标
			tp_dev.y[4]=tp_dev.y[0];  	   			 
		}			   
	}else
	{
		if(tp_dev.sta&TP_PRES_DOWN)//之前是被按下的
		{
			tp_dev.sta&=~(1<<7);//标记按键松开
		}else//之前就没有被按下
		{
			tp_dev.x[4]=0;
			tp_dev.y[4]=0;
			tp_dev.x[0]=0xffff;
			tp_dev.y[0]=0xffff;
		}	    
	}
	return tp_dev.sta&TP_PRES_DOWN;//返回当前的触屏状态
}


//////////////////////////////////////////////////////////////////////////	 
//保存在EEPROM里面的地址区间基址,占用14个字节(RANGE:SAVE_ADDR_BASE~SAVE_ADDR_BASE+13)
#define SAVE_ADDR_BASE 40
//保存校准参数										    
void TP_Save_Adjdata(void)
{
//	AT24CXX_Write(SAVE_ADDR_BASE,(uint8_t*)&tp_dev.xfac,14);	//强制保存&tp_dev.xfac地址开始的14个字节数据，即保存到tp_dev.touchtype
// 	AT24CXX_WriteOneByte(SAVE_ADDR_BASE+14,DFT_SCAN_DIR);		//在最后，写DFT_SCAN_DIR标记校准方向
}

//得到保存在EEPROM里面的校准值
//返回值：1，成功获取数据
//        0，获取失败，要重新校准
uint8_t TP_Get_Adjdata(void)
{					  
//	uint8_t temp;
//	temp=AT24CXX_ReadOneByte(SAVE_ADDR_BASE+14);//读取标记字,看是否校准过！ 		 
//	if(temp==DFT_SCAN_DIR)//触摸屏已经校准过了			   
// 	{ 
//		AT24CXX_Read(SAVE_ADDR_BASE,(uint8_t*)&tp_dev.xfac,14);//读取之前保存的校准数据  
//		return 1;	 
//	}
	return 0;
}	 

//触摸屏校准代码
//得到四个校准参数
void TP_Adjust(void)
{
	uint16_t pos_temp[4][2];//坐标缓存值
	uint8_t  cnt=0;	
	uint16_t d1,d2;
	uint32_t tem1,tem2;
	double fac; 	
 	cnt=0;
  LCD_Fill(0,0,lcddev.width,lcddev.height,WHITE);//清屏   
  LCD_ShowString(5,40,"Please adjustment the screen.",RED,WHITE,16,0); //显示提示信息
	TP_Drow_Touch_Point(20,20,RED);//画点1 
	tp_dev.sta=0;//消除触发信号 
	tp_dev.xfac=0;//xfac用来标记是否校准过,所以校准之前必须清掉!以免错误	 
	while(1)//如果连续10秒钟没有按下,则自动退出
	{
		tp_dev.scan(1);//扫描物理坐标
		if((tp_dev.sta&0xc0)==TP_CATH_PRES)//按键按下了一次(此时按键松开了.)
		{
			tp_dev.sta&=~(1<<6);//标记按键已经被处理过了.
			
			pos_temp[cnt][0]=tp_dev.x[0];
			pos_temp[cnt][1]=tp_dev.y[0];
			cnt++;	  
			switch(cnt)
			{			   
				case 1:						 
					TP_Drow_Touch_Point(20,20,WHITE);				//清除点1 
					TP_Drow_Touch_Point(lcddev.width-20,20,RED);	//画点2
					break;
				case 2:
 					TP_Drow_Touch_Point(lcddev.width-20,20,WHITE);	//清除点2
					TP_Drow_Touch_Point(20,lcddev.height-20,RED);	//画点3
					break;
				case 3:
 					TP_Drow_Touch_Point(20,lcddev.height-20,WHITE);			//清除点3
 					TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,RED);	//画点4
					break;
				case 4:	 //全部四个点已经得到
	    		    //对边相等
				  printf("Point1: X=%d, Y=%d\n", pos_temp[0][0], pos_temp[0][1]);
          printf("Point2: X=%d, Y=%d\n", pos_temp[1][0], pos_temp[1][1]);
          printf("Point3: X=%d, Y=%d\n", pos_temp[2][0], pos_temp[2][1]);
          printf("Point4: X=%d, Y=%d\n", pos_temp[3][0], pos_temp[3][1]);
				
					tem1=abs(pos_temp[0][0]-pos_temp[1][0]);//x1-x2
					tem2=abs(pos_temp[0][1]-pos_temp[1][1]);//y1-y2
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,2的距离
					
					tem1=abs(pos_temp[2][0]-pos_temp[3][0]);//x3-x4
					tem2=abs(pos_temp[2][1]-pos_temp[3][1]);//y3-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到3,4的距离
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05||d1==0||d2==0)//不合格
					{
						cnt=0;
 				    TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
   	 				TP_Drow_Touch_Point(20,20,RED);								//画点1
						LCD_ShowString(5,40,"Touch Adjust Filed!          ",RED,WHITE,16,0);//校正失败提示
						LCD_ShowString(5,60,"Please Adjust Again!         ",RED,WHITE,16,0);//校正失败提示
						HAL_Delay(1000);
 						continue;
					}
					tem1=abs(pos_temp[0][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[0][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,3的距离
					
					tem1=abs(pos_temp[1][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[1][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到2,4的距离
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)//不合格
					{
						cnt=0;
 				    TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
   	 				TP_Drow_Touch_Point(20,20,RED);								//画点1
						LCD_ShowString(5,40,"Touch Adjust Filed!          ",RED,WHITE,16,0);//校正失败提示
						LCD_ShowString(5,60,"Please Adjust Again!         ",RED,WHITE,16,0);//校正失败提示
						HAL_Delay(1000);
						continue;
					}//正确了
								   
					//对角线相等
					tem1=abs(pos_temp[1][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[1][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,4的距离
	
					tem1=abs(pos_temp[0][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[0][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到2,3的距离
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)//不合格
					{
						cnt=0;
 				    TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
   	 				TP_Drow_Touch_Point(20,20,RED);								//画点1
						LCD_ShowString(5,40,"Touch Adjust Filed!          ",RED,WHITE,16,0);//校正失败提示
						LCD_ShowString(5,60,"Please Adjust Again!         ",RED,WHITE,16,0);//校正失败提示
						HAL_Delay(1000);
 							continue;
					}//正确了
					//计算结果
					tp_dev.xfac=(float)(lcddev.width-40)/(pos_temp[1][0]-pos_temp[0][0]);//得到xfac		 
					tp_dev.xoff=(lcddev.width-tp_dev.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;//得到xoff
						  
					tp_dev.yfac=(float)(lcddev.height-40)/(pos_temp[2][1]-pos_temp[0][1]);//得到yfac
					tp_dev.yoff=(lcddev.height-tp_dev.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;//得到yoff  
					
					TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
					LCD_ShowString(5,40,"Touch Screen Adjust OK!      ",RED,WHITE,16,0);//校正完成
					LCD_ShowString(5,60,"                             ",RED,WHITE,16,0);//清除文字
					HAL_Delay(1000);
 					LCD_ShowString(5,40,"                       ",RED,WHITE,16,0);//清除文字
					TP_Save_Adjdata();////保存校准参数	
					return;//校正完成				 
			}
		}
 	}
}
//触摸屏初始化  		    
uint8_t TP_Init(void)
{

	TP_Read_XY(&tp_dev.x[0],&tp_dev.y[0]);//第一次读取初始化	 
//	AT24CXX_Init();			//初始化24CXX
//	if(TP_Get_Adjdata())return 0;//已经校准
//	else			  		//未校准?
//	{ 										    
		TP_Adjust();  		//屏幕校准  
//	}			
	TP_Get_Adjdata();	
	return 1;
}



void Touch_Test(void)
{
	uint16_t lastpos[2];//最后一次的数据
	lastpos[0]=0xffff;
	TP_Init();//触摸初始化
	LCD_Clear(WHITE);
	while(1)
	{
		tp_dev.scan(0);//扫描
		if(tp_dev.sta&TP_PRES_DOWN)//有按键被按下
		{				  
			HAL_Delay(1);//必要的延时,否则老认为有按键按下.		    
			if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{
				if(lastpos[0]==0XFFFF)
				{
					 lastpos[0]=tp_dev.x[0];
					 lastpos[1]=tp_dev.y[0];
				}
				LCD_DrawRoughLine(lastpos[0],lastpos[1],tp_dev.x[0],tp_dev.y[0],RED);//画线
				lastpos[0]=tp_dev.x[0];
				lastpos[1]=tp_dev.y[0];
				LCD_ShowString(10,lcddev.height-60,"X:",RED,BLACK,16,0);
				LCD_ShowIntNum(26,lcddev.height-60,tp_dev.x[0],3,RED,BLACK,16);
				LCD_ShowString(10,lcddev.height-40,"Y:",RED,BLACK,16,0);
				LCD_ShowIntNum(26,lcddev.height-40,tp_dev.y[0],3,RED,BLACK,16);
			}    
		}
  }
}

