/* �����ļ� */
#include "def.h"
#include "2410lib.h"
#include "option.h"
#include "2410addr.h"
#include "interrupt.h"

/********************************************************************
// Function name	: Main
// Description	    : JXARM9-2410 IIS¼��/����ʵ��������
//                    ʵ�ֹ��ܣ�
//                        ʵ��JXRAM9-2410��PC���Ĵ���ͨѶ
//                        JXARM9-2410 UART0 <==> PC COM
// Return type		: void
// Argument         : void
*********************************************************************/
void Main(void)
{
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
	PRINTF("\n---IIS¼��/�������Գ���---\n");
	PRINTF("\n�뽫UART0��PC���ڽ������ӣ�Ȼ�����������ն˳���(115200, 8, N, 1)\n");
	
#ifdef  IIS_PLAY_AND_RECORD	
	Record_Iis();
#else
	Play_Iis();
#endif
	while(1)
	{
		Play_Iis();
	}
}

