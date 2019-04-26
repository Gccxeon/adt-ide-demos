/* �����ļ� */
#include "def.h"
#include "2410lib.h"
#include "option.h"
#include "2410addr.h"
#include "interrupt.h"

extern int data_len;

/********************************************************************
// Function name	: Main
// Description	    : JXARM9-2410 tftpͨ��ʵ��������
// Return type		: void
// Argument         : void
*********************************************************************/
void Main(void)
{
	unsigned char *ip_s;
	unsigned long ip;

	/* ����ϵͳʱ�� */
    ChangeClockDivider(2,1);
    U32 mpll_val = 0 ;
    mpll_val = (92<<12)|(1<<4)|(1);
    ChangeMPllValue((mpll_val>>12)&0xff, (mpll_val>>4)&0x3f, mpll_val&3); 
    
    /* ��ʼ���˿� */
    Port_Init();
    
    /* ��ʼ������ */
    Uart_Init(0,115200);
    Uart_Select(0);
    
    /* ��ӡ��ʾ��Ϣ */
	PRINTF("\n---tftp���Գ���---\n");
	PRINTF("\n�뽫UART0��PC���ڽ������ӣ�Ȼ�����������ն˳���(115200, 8, N, 1)\n");
		
	ip_s=(unsigned char *)&ip;
	ip_s[3]=192;	
	ip_s[2]=168;
	ip_s[1]=1;
	ip_s[0]=46;
	
	tftp_main(ip, 0x30100000);
	
	Uart_Printf("\ntotal = %d bytes.\r", data_len);
	while(1)
	{
	}
}
