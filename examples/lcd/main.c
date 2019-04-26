/* �����ļ� */
#include "def.h"
#include "2410lib.h"
#include "option.h"
#include "2410addr.h"
#include "interrupt.h"
#include "lcdlib.h"

//#define STN_LCD
#define TFT_8_0

void (*PutPixel)(U32,U32,U32);
void Lcd_Disp_Char(void);
void Lcd_Disp_Grap(void);
/********************************************************************
// Function name	: Main
// Description	    : JXARM9-2410 LCD��ʾʵ��������
//                    ʵ�ֹ��ܣ�
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
	PRINTF("\n---LCD���Գ���---\n");
	PRINTF("\n�뽫UART0��PC���ڽ������ӣ�Ȼ�����������ն˳���(115200, 8, N, 1)\n");

	/* LCD��ʼ�� */
    Lcd_Port_Init();
#ifdef STN_LCD
    Lcd_Init(MODE_CSTN_8BIT);
    Glib_Init(MODE_CSTN_8BIT);
    Lcd_CstnOnOff(1);
    
    Glib_ClearScr(0xff, MODE_CSTN_8BIT);
#else
	#ifdef TFT_8_0
		rGPCCON &= ~(3<<8);
		rGPCCON |= (2<<8);

	    Lcd_Init(MODE_TFT_16BIT_640480);
	    Glib_Init(MODE_TFT_16BIT_640480);
	
	    Glib_ClearScr(0xffff, MODE_TFT_16BIT_640480);
	    Lcd_PowerEnable(0, 1);
	    Lcd_EnvidOnOff(1);  	
	#else
	    Lcd_Init(MODE_TFT_16BIT_240320);
	    Glib_Init(MODE_TFT_16BIT_240320);
	    
	    Glib_ClearScr(0xffff, MODE_TFT_16BIT_240320);
	    Lcd_PowerEnable(0, 1);
	    Lcd_EnvidOnOff(1);
    #endif
#endif
    while(1)
    {
//#define LCD_DISP_CHAR	
#ifdef  LCD_DISP_CHAR	
	Lcd_Disp_Char();
#else
	Lcd_Disp_Grap();
#endif
    }
}    

void Lcd_Disp_Char(void)
{
	/* ��ʾ�ַ��� */
 	Glib_disp_hzk16(30,100,"�人��ά����Ϣ�������޹�˾", 0x0);
	while(1);
}

void Lcd_Disp_Grap(void)
{
	int i,j;
	
    for(j=0;j<480;j++)
  	  for(i=0;i<640;i++)	//RRRGGGBB
	    (*PutPixel)(i,j,0xf800);
	    
	Delay (100);   	   
   
    for(j=0;j<480;j++)
  	  for(i=0;i<640;i++)	//RRRGGGBB
	    (*PutPixel)(i,j,0x07e0);
   
	Delay (100);   	   

    for(j=0;j<480;j++)
  	  for(i=0;i<640;i++)	//RRRGGGBB
	    (*PutPixel)(i,j,0x001f);
   
	Delay (100);   	   

    for(j=0;j<480;j++)
  	  for(i=0;i<640;i++)	//RRRGGGBB
	    (*PutPixel)(i,j,0xffff);
   
	Delay (100);   	   
	/*int i,j;

	PRINTF("\ndis 1\n");
	for(i=0;i<640;i++)
    	for(j=0;j<480;j++)
	    	_PutTft16Bit_640480(i,j,*(short *)(0xf100 + (j*640 + i) * 2));*/
}

