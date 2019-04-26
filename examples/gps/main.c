/****************************************************************************/
/*                                                                          */
/* FILE NAME                                      VERSION                   */
/*                                                                          */
/* GPRS.C                                            1.0                    */
/*                                                                          */
/* DESCRIPTION                                                              */
/*                                                                          */
/*     JX44B0(S3C44B0X)GPSȫ��λʵ��                                      */
/*                                                                          */
/*                                                                          */
/* DATA STRUCTURES                                                          */
/*                                                                          */
/* FUNCTIONS :                                                              */
/*     ��JX44B0��ѧʵ�������GPSȫ��λʵ��                                */
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
/* ѧϰJX44B0��ȫ��λ���ܵ�ʵ�ַ�����                                     */
/* ע�⣺                                                                   */
/*     1. ��ʵ�����������JX44B0-3ʵ����                                    */
/*     2. ʵ��֮ǰ���Ķ��û��ֲᣬ��������ȷ��Ӳ������                      */
/*     3. ʵ����̵Ĵ���ʹ�ô���1�������Ǵ���0����������ʵ���в��        */
/*     ���Ҳ����ʱ�������Ϊ4800                                             */
/****************************************************************************/

/* �����ļ� */
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
// Description	    : GPSʹ�ô���0������1������ʾ��GPS��ʼ��֮ǰ��
//                    ���øú������г�ʼ��
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
// Description	    : ��GPS�����ַ�
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
// Description	    : ����GPS��λ��Ϣ
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
// Description	    : ��ӡ���Ƕ�λ��Ϣ
// Return type		: void
// Argument         : GPSINFO * pinfo
*********************************************************************/
void TRACE_MSG(GPSINFO * pinfo)
{
	Uart_Select(1);
	Uart_Printf("UTCʱ�䣺%dʱ%d��%d��%d����\n", pinfo->hour, pinfo->min, pinfo->sec, pinfo->secFrac);
	Uart_Printf("����ʱ�䣺%dʱ%d��%d��%d����\n", pinfo->bjhour, pinfo->min, pinfo->sec, pinfo->secFrac);
	Uart_Printf("γ�ȣ�%sγ%f\n", (pinfo->latNS == 'N' ? "��" : "��"), pinfo->latitude);
	Uart_Printf("���ȣ�%s��%f\n", (pinfo->lgtEW == 'E' ? "��" : "��"), pinfo->longitud);
	Uart_Printf("\n");
}
/********************************************************************
// Function name	: gps_proc
// Description	    : ����GPS��λ��Ϣ������
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
  		
  		// GPS��λ��Ϣ��ȡ
    	gps_recv_cmd(cmd_str);
    		
		// GPS��λ��Ϣ����
		GPSReceive(&info, cmd_str, strlen(cmd_str));
		
		// ��ӡ��λ��Ϣ
		if(info.bIsGPGGA ==1)
			TRACE_MSG(&info);
	}
}
/********************************************************************
// Function name	: gps_init
// Description	    : GPSģ���ʼ����������4800
// Return type		: void
// Argument         : 
*********************************************************************/
void gps_init()
{
	/* ����ϵͳʱ�� */
    ChangeClockDivider(1,1);          // 1:2:4    
    ChangeMPllValue(0xa1,0x3,0x1);    // FCLK=202.8MHz  
    
    /* ��ʼ���˿� */
    Port_Init();
    
    /* ��ʼ������ */
    Uart_Init(0,9600);
    Uart_Select(0);
	
	gps_uart_ctrl(0x2);
}
/********************************************************************
// Function name	: Main
// Description	    : ������
// Return type		: void
// Argument         : 
*********************************************************************/
void Main()
{
	char ch;
	
	/* ����ϵͳʱ�� */
    ChangeClockDivider(2,1);
    unsigned int mpll_val = 0 ;
    mpll_val = (92<<12)|(1<<4)|(1);
    ChangeMPllValue((mpll_val>>12)&0xff, (mpll_val>>4)&0x3f, mpll_val&3); 
	
	/* �жϳ�ʼ�� */
    Isr_Init();
    /* ��ʼ���˿� */
    Port_Init();
    
    // GPS��ʼ��
	gps_init();
	
	Uart_Select(1);
	Uart_Printf("GPS Test!\n");
		
	// ��ʼGPS����
	gps_proc();
}
