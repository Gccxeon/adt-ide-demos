/* �����ļ� */
#include "def.h"
#include "2410lib.h"
#include "option.h"
#include "2410addr.h"
#include "interrupt.h"

/* functions */
void eint2_isr(void) __attribute__ ((interrupt("IRQ")));;
void eint3_isr(void) __attribute__ ((interrupt("IRQ")));;
void delay();

/* variables */
int dither_count2 = 0;
int dither_count3 = 0;
static int nLed = 0;

/*****************************************************************************
// Function name	: Main
// Description	    : JXARM9-2410 �ж�ʵ��������
//                    ��ɹ���:
//                        �ⲿ�жϰ��������ж�
// Return type		: void
// Argument         : void
*****************************************************************************/
void Main(void)
{
	/* ����ϵͳʱ�� */
    ChangeClockDivider(2,1);
    U32 mpll_val = 0 ;
    mpll_val = (92<<12)|(1<<4)|(1);
    ChangeMPllValue((mpll_val>>12)&0xff, (mpll_val>>4)&0x3f, mpll_val&3); 
	
	/* �жϳ�ʼ�� */
    Isr_Init();
    /* ��ʼ���˿� */
    Port_Init();
    
    /* ��ʼ������ */
    Uart_Init(0,115200);
    Uart_Select(0);

    /* ��ӡ��ʾ��Ϣ */
	PRINTF("\n---�ⲿ�жϲ��Գ���---\n");
	PRINTF("\n�뽫UART0��PC���ڽ������ӣ�Ȼ�����������ն˳���(115200, 8, N, 1)\n");
	PRINTF("\n�ⲿ�жϲ��Կ�ʼ\n");

	/* �����ж� */
	Irq_Request(IRQ_EINT3, eint3_isr);

    /* ʹ���ж� */
    Irq_Enable(IRQ_EINT3);
       	
    dither_count2 = 0;
    dither_count3 = 0;
    while(1)
    {
    	delay();
    	dither_count2++;
    	dither_count3++;
    }
}

/*****************************************************************************
// Function name	: eint2_isr
// Description	    : EINT2�жϴ�������
// Return type		: int
// Argument         : void
*****************************************************************************/
void eint2_isr(void)
{
	Irq_Clear(IRQ_EINT2);        

    if(dither_count2 > 10) 
    {
	   	dither_count2 = 0;
	
		Led_Display(nLed);

		nLed = (nLed ^ 0x01);
	}
}

/*****************************************************************************
// Function name	: eint3_isr
// Description	    : EINT3�жϴ�������
// Return type		: int
// Argument         : void
*****************************************************************************/
void eint3_isr(void)
{
	Irq_Clear(IRQ_EINT3);        

    if(dither_count3 > 10) 
    {
	   	dither_count3 = 0;
	
		Led_Display(nLed);

		nLed = (nLed ^ 0x02);
	}
}

void delay()
{
	int index = 0; 
	
	for ( index = 0 ; index < 20000; index++);
}