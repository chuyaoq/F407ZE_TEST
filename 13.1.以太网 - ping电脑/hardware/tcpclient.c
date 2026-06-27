/***********************************************************************
文件名称：TCP_CLIENT.C
功    能：完成TCP的客户端的数据收发
编写时间：2013.4.25
编 写 人：
注    意：
***********************************************************************/

#include "main.h"
#include "tcp.h"
#include "lwip.h"
#include "ip_addr.h" 
#include "tcpclient.h"
#include "stdio.h"

#define TCP_LOCAL_PORT     		8887 
#define TCP_SERVER_PORT    		8888
#define TCP_SERVER_IP   		  192,168,0,200

extern struct tcp_pcb *tcp_client_pcb;

void TCP_Client_Init(u16_t local_port,u16_t remote_port,unsigned char a,unsigned char b,unsigned char c,unsigned char d);
err_t TCP_Client_Send_Data(struct tcp_pcb *cpcb,unsigned char *buff,unsigned int length);
struct tcp_pcb *Check_TCP_Connect(void);
void Delay_s(unsigned long ulVal); /* 利用循环产生一定的延时 */

typedef unsigned short u16_t;

struct tcp_pcb *tcp_client_pcb;
unsigned char connect_flag = 0;


/***********************************************************************
函数名称：void TCP_Client_Send_Data(unsigned char *buff)
功    能：TC[客户端发送数据函数
输入参数：
输出参数：
编写时间：2013.4.25
编 写 人：
注    意：for(cpcb = tcp_active_pcbs;cpcb != NULL; cpcb = cpcb->next) 
***********************************************************************/
err_t TCP_Client_Send_Data(struct tcp_pcb *cpcb,unsigned char *buff,unsigned int length)
{
	err_t err;

	err = tcp_write(cpcb,buff,length,TCP_WRITE_FLAG_COPY);	//发送数据
	tcp_output(cpcb);
	//tcp_close(tcp_client_pcb);				//发送完数据关闭连接,根据具体情况选择使用	
	return err;					
}
extern union tcp_listen_pcbs_t tcp_listen_pcbs;
extern struct tcp_pcb *tcp_active_pcbs;  /* List of all TCP PCBs that are in a
\***********************************************************************
函数名称：Check_TCP_Connect(void)
功    能：检查连接
输入参数：
输出参数：
编写时间：2013.4.25
编 写 人：
注    意：for(cpcb = tcp_active_pcbs;cpcb != NULL; cpcb = cpcb->next) 
***********************************************************************/
struct tcp_pcb *Check_TCP_Connect(void)
{
	struct tcp_pcb *cpcb = 0;
	connect_flag = 0;
	for(cpcb = tcp_active_pcbs;cpcb != NULL; cpcb = cpcb->next)
	{
	//	if(cpcb->local_port == TCP_LOCAL_PORT && cpcb->remote_port == TCP_SERVER_PORT)		//如果TCP_LOCAL_PORT端口指定的连接没有断开
		if(cpcb -> state == ESTABLISHED)  //如果得到应答，则证明已经连接上
		{
			
			connect_flag = 1;  						//连接标志
			break;							   	
		}
	}

	if(connect_flag == 0)  	// TCP_LOCAL_PORT指定的端口未连接或已断开
	{
		TCP_Client_Init(TCP_LOCAL_PORT,TCP_SERVER_PORT,TCP_SERVER_IP); //重新连接
		cpcb = 0;
	}
	return cpcb;	
}
/***********************************************************************
函数名称：err_t RS232_TCP_Connected(void *arg,struct tcp_pcb *pcb,err_t err)
功    能：完成RS232到TCP的数据发送
输入参数：
输出参数：
编写时间：2013.4.25
编 写 人：
注    意：这是一个回调函数，当TCP客户端请求的连接建立时被调用
***********************************************************************/
err_t TCP_Connected(void *arg,struct tcp_pcb *pcb,err_t err)
{
	//tcp_client_pcb = pcb;
	return ERR_OK;
}
/***********************************************************************
函数名称：TCP_Client_Recv(void *arg, struct tcp_pcb *pcb,struct pbuf *p,err_t err)
功    能：tcp客户端接收数据回调函数
输入参数：
输出参数：
编写时间：2013.4.25
编 写 人：
注    意：这是一个回调函数，当TCP服务器发来数据时调用
***********************************************************************/

err_t  TCP_Client_Recv(void *arg, struct tcp_pcb *pcb,struct pbuf *p,err_t err)
{
	struct pbuf *p_temp = p;
	
	if(p_temp != NULL)
	{	
		tcp_recved(pcb, p_temp->tot_len);//获取数据长度 tot_len：tcp数据块的长度
		while(p_temp != NULL)	
		{				
			/******将数据原样返回*******************/
			tcp_write(pcb,p_temp->payload,p_temp->len,TCP_WRITE_FLAG_COPY); 	// payload为TCP数据块的起始位置       
			tcp_output(pcb);
			p_temp = p_temp->next;
		}		
	}
	else
	{
		tcp_close(pcb); 											/* 作为TCP服务器不应主动关闭这个连接？ */
	}
	/* 释放该TCP段 */
	pbuf_free(p); 	
	err = ERR_OK;
	return err;
}
/***********************************************************************
函数名称：TCP_Client_Init(u16_t local_port,u16_t remote_port,unsigned char a,unsigned char b,unsigned char c,unsigned char d)
功    能：tcp客户端初始化
输入参数：local_port本地端口号；remote_port：目标端口号；a,b,c,d：服务器ip
输出参数：
编写时间：2013.4.25
编 写 人：
注    意：
***********************************************************************/
void TCP_Client_Init(u16_t local_port,u16_t remote_port,unsigned char a,unsigned char b,unsigned char c,unsigned char d)
{static int is_initialized = 0;  // 防止重复初始化

    if (is_initialized)
    {
        printf("TCP client already initialized\n");
        return;
    }

    ip4_addr_t ipaddr;
    err_t err;

    // 释放旧PCB
    if (tcp_client_pcb != NULL)
    {
        tcp_close(tcp_client_pcb);
        tcp_client_pcb = NULL;
    }

    IP4_ADDR(&ipaddr, a, b, c, d);

    // 创建PCB
    tcp_client_pcb = tcp_new();
    if (tcp_client_pcb == NULL)
    {
        printf("error1: tcp_new() failed - out of memory\n");
        return;
    }

    // 绑定端口
    err = tcp_bind(tcp_client_pcb, IP_ADDR_ANY, local_port);
    if (err != ERR_OK)
    {
        printf("error2: tcp_bind() failed - err=%d\n", err);
        tcp_close(tcp_client_pcb);
        tcp_client_pcb = NULL;
        return;
    }

    // 连接服务器
    err = tcp_connect(tcp_client_pcb, &ipaddr, remote_port, TCP_Connected);
    if (err != ERR_OK)
    {
        printf("error3: tcp_connect() failed - err=%d\n", err);
        tcp_close(tcp_client_pcb);
        tcp_client_pcb = NULL;
        return;
    }

    // 接受回调
    tcp_recv(tcp_client_pcb, TCP_Client_Recv);

    is_initialized = 1;
    printf("TCP client initialized successfully\n");
}



unsigned char tcp_data[] = "ccc_Tcptest!\n";
struct tcp_pcb *pcb;
void StartDefaultTask(void)
{
//	MX_LWIP_Init();
//	TCP_Client_Init(TCP_LOCAL_PORT,TCP_SERVER_PORT,TCP_SERVER_IP); //tcp客户端初始化
//  for(;;)
//  {
//			osDelay(1000);
//			pcb = Check_TCP_Connect();
//			if(pcb != 0)
//			{
//					printf("senddata");
//					TCP_Client_Send_Data(pcb,tcp_data,sizeof(tcp_data));	//该函数为主动向服务器发送函数，
//			}
//  }
}

