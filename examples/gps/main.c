/****************************************************************************/
/*                                                                          */
/* FILE NAME                                      VERSION                   */
/*                                                                          */
/* GPRS.C                                            1.0                    */
/*                                                                          */
/* DESCRIPTION                                                              */
/*                                                                          */
/*     JX44B0(S3C44B0X)GPS全球定位实验                                      */
/*                                                                          */
/*                                                                          */
/* DATA STRUCTURES                                                          */
/*                                                                          */
/* FUNCTIONS :                                                              */
/*     在JX44B0教学实验箱进行GPS全球定位实验                                */
/*                                                                          */
/* DEPENDENCIES                                                             */
/*     JX44B0-3                                                             */
/*                                                                          */
/*                                                                          */
/* NAME:                                                                    */
/* REMARKS:                                                                 */
/*                                                                          */
/*								Copyright (C) 2003 Wuhan CVTECH CO.,LTD     */
/****************************************************************************/

/****************************************************************************/
/* 学习JX44B0中全球定位功能的实现方法：                                     */
/* 注意：                                                                   */
/*     1. 该实验仅仅适用于JX44B0-3实验箱                                    */
/*     2. 实验之前请阅读用户手册，并进行正确的硬件连接                      */
/*     3. 实验过程的串口使用串口1，而不是串口0，这与其他实验有差别，        */
/*     而且波特率必须设置为4800                                             */
/****************************************************************************/

/* 包含文件 */
#include "2410addr.h"
#include "2410lib.h"
#include "gpslib.h"

/* defines */
#define GPS_CONTROL_ADDR       0x28000004
#define GPS_CONTROL_MASK_UART   (3<<0)
#define GPS_RECV_CMD_MAX_BUF 10*1024

/* globals */ 
int gps_ctrl_value = 0x0;
char gps_recv_buf[GPS_RECV_CMD_MAX_BUF];
int  gps_recv_read = 0;
int  gps_recv_write = 0;

/********************************************************************
// Function name	: gps_uart_ctrl
// Description	    : GPS使用串口0，串口1用于显示，GPS初始化之前请
//                    调用该函数进行初始化
// Return type		: void
// Argument         : int uart
*********************************************************************/
void gps_uart_ctrl(int uart)
{
	gps_ctrl_value &= ~GPS_CONTROL_MASK_UART;
	gps_ctrl_value |= uart;
	
	*(unsigned char *)GPS_CONTROL_ADDR = gps_ctrl_value;
}
/********************************************************************
// Function name	: gps_recv_char
// Description	    : 从GPS接收字符
// Return type		: void
// Argument         : 
*********************************************************************/
void gps_recv_char()
{
	char ch;
	
	// select uart 0
	Uart_Select(0);
	
	// receive command
	ch = Uart_GetKey();
	if(ch == 0)
	{
		return;
	}else
	{
		gps_recv_buf[gps_recv_write] = ch;
		gps_recv_write ++;
		if(gps_recv_write >= GPS_RECV_CMD_MAX_BUF)
			gps_recv_write = 0;
	}
}
/********************************************************************
// Function name	: gps_recv_cmd
// Description	    : 接收GPS定位信息
// Return type		: void
// Argument         : char *cmd
*********************************************************************/
void gps_recv_cmd(char *cmd)
{
	int loopcnt = 0;
	while(1)
	{	
		if(gps_recv_read == gps_recv_write)
		{
			gps_recv_char();
			continue;
		}
		cmd[loopcnt ++] = gps_recv_buf[gps_recv_read];
		if( (gps_recv_buf[gps_recv_read] == '\r')  \
          )
		{
			gps_recv_read ++;
			if(gps_recv_read >= GPS_RECV_CMD_MAX_BUF)
				gps_recv_read = 0;
			cmd[loopcnt ++] = 0;
			break;
		}
		gps_recv_read ++;
		if(gps_recv_read >= GPS_RECV_CMD_MAX_BUF)
			gps_recv_read = 0;
	}
}
/********************************************************************
// Function name	: TRACE_MSG
// Description	    : 打印卫星定位信息
// Return type		: void
// Argument         : GPSINFO * pinfo
*********************************************************************/
void TRACE_MSG(GPSINFO * pinfo)
{
	Uart_Select(1);
	Uart_Printf("UTC时间：%d时%d分%d秒%d毫秒\n", pinfo->hour, pinfo->min, pinfo->sec, pinfo->secFrac);
	Uart_Printf("北京时间：%d时%d分%d秒%d毫秒\n", pinfo->bjhour, pinfo->min, pinfo->sec, pinfo->secFrac);
	Uart_Printf("纬度：%s纬%f\n", (pinfo->latNS == 'N' ? "北" : "南"), pinfo->latitude);
	Uart_Printf("经度：%s经%f\n", (pinfo->lgtEW == 'E' ? "东" : "西"), pinfo->longitud);
	Uart_Printf("\n");
}
/********************************************************************
// Function name	: gps_proc
// Description	    : 接收GPS定位信息并解析
// Return type		: void
// Argument         : 
*********************************************************************/
void gps_proc()
{
	char cmd_str[1024];
	char *pstr;
	while (1)
  	{  
  		GPSINFO info;
  		
  		// GPS定位信息提取
    	gps_recv_cmd(cmd_str);
    		
		// GPS定位信息解析
		GPSReceive(&info, cmd_str, strlen(cmd_str));
		
		// 打印定位信息
		if(info.bIsGPGGA ==1)
			TRACE_MSG(&info);
	}
}
/********************************************************************
// Function name	: gps_init
// Description	    : GPS模块初始化，波特率4800
// Return type		: void
// Argument         : 
*********************************************************************/
void gps_init()
{
	/* 配置系统时钟 */
    ChangeClockDivider(1,1);          // 1:2:4    
    ChangeMPllValue(0xa1,0x3,0x1);    // FCLK=202.8MHz  
    
    /* 初始化端口 */
    Port_Init();
    
    /* 初始化串口 */
    Uart_Init(0,9600);
    Uart_Select(0);
	
	gps_uart_ctrl(0x2);
}
/********************************************************************
// Function name	: Main
// Description	    : 主函数
// Return type		: void
// Argument         : 
*********************************************************************/
void Main()
{
	char ch;
	
	/* 配置系统时钟 */
    ChangeClockDivider(2,1);
    unsigned int mpll_val = 0 ;
    mpll_val = (92<<12)|(1<<4)|(1);
    ChangeMPllValue((mpll_val>>12)&0xff, (mpll_val>>4)&0x3f, mpll_val&3); 
	
	/* 中断初始化 */
    Isr_Init();
    /* 初始化端口 */
    Port_Init();
    
    // GPS初始化
	gps_init();
	
	Uart_Select(1);
	Uart_Printf("GPS Test!\n");
		
	// 开始GPS处理
	gps_proc();
}
