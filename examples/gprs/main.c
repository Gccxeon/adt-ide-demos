/****************************************************************************/
/*                                                                          */
/* FILE NAME                                      VERSION                   */
/*                                                                          */
/* GPRS.C                                            1.0                    */
/*                                                                          */
/* DESCRIPTION                                                              */
/*                                                                          */
/*     JX44B0(S3C44B0X)GPRSͨѶʵ��                                         */
/*                                                                          */
/*                                                                          */
/* DATA STRUCTURES                                                          */
/*                                                                          */
/* FUNCTIONS :                                                              */
/*     ��JX44B0��ѧʵ�������GPRSͨѶʵ��                                   */
/*                                                                          */
/* DEPENDENCIES                                                             */
/*     JX44B0-2                                                             */
/*     JX44B0-3                                                             */
/*                                                                          */
/*                                                                          */
/* NAME:                                                                    */
/* REMARKS:                                                                 */
/*                                                                          */
/*								Copyright (C) 2003 Wuhan CVTECH CO.,LTD     */
/****************************************************************************/

/****************************************************************************/
/* ѧϰJX44B0�е绰���й��ܺ���Ӣ�Ķ��Ź��ܵ�ʵ�ַ�����                     */
/* ע�⣺                                                                   */
/*     1. ��ʵ�����������JX44B0-2�Լ�JX44B0-3ʵ����                        */
/*     2. ʵ��֮ǰ���Ķ��û��ֲᣬ��������ȷ��Ӳ������                      */
/*     3. ʵ�������ҪSIM����SIM����ʹ���й��ƶ����й���ͨ�ĸ����ֻ���      */
/*     4. SIM���벻Ҫ�����Σ��������׵����տ�                             */
/*     5. ����ʵ������Ҫ�޸Ķ������ĺ��룬����������ֻ��е��������ø�ֵ��  */
/*     ע��ȥ��ǰ���'+'��                                                  */
/****************************************************************************/

/* �����ļ� */
#include "2410addr.h"
#include "2410lib.h"
#include "gprs.h"

/* ���������� */
char gprs_key_recv_buf[GPRS_RECV_CMD_MAX_BUF];
int  gprs_key_recv_read = 0;
int  gprs_key_recv_write = 0;

/********************************************************************
// Function name	: TRACE
// Description	    : �ڴ���0�ϴ�ӡ������Ϣ
// Return type		: void
// Argument         : char *string
*********************************************************************/
void TRACE(char *string)
{
	// disable int
//	gprs_disable_int();
	
	Uart_Select(0);
	Uart_Printf(string);
	
	// enable int
//	gprs_enable_int();
}
/********************************************************************
// Function name	: gprs_recv_key
// Description	    : ����ȡ�ļ�ֵ���밴��������
// Return type		: void
// Argument         : int key
*********************************************************************/
void gprs_recv_key(int key)
{
	gprs_key_recv_buf[gprs_key_recv_write] = key;
	gprs_key_recv_write ++;
	if(gprs_key_recv_write >= GPRS_RECV_CMD_MAX_BUF)
		gprs_key_recv_write = 0;
		
	if(gprs_key_recv_write == gprs_key_recv_read)
	{
		// ����������
		gprs_key_recv_read ++;
		if(gprs_key_recv_read >= GPRS_RECV_CMD_MAX_BUF)
			gprs_key_recv_read = 0;
	}
}
/********************************************************************
// Function name	: key_get_char
// Description	    : ����ɨ����
// Return type		: char
// Argument         : int row
// Argument         : int col
*********************************************************************/
char key_get_char(int row, int col)
{
	char key = 0;
	
	switch( row )
	{
	case 0:
		if((col & 0x01) == 0) key = '0'; 
		else if((col & 0x02) == 0) key = 'A'; 
		else if((col & 0x04) == 0) key = 'B'; 
		else if((col & 0x08) == 0) key = 'F'; 
		break;
	case 1:
		if((col & 0x01) == 0) key = '7'; 
		else if((col & 0x02) == 0) key = '8'; 
		else if((col & 0x04) == 0) key = '9';
		else if((col & 0x08) == 0) key = 'E';
		break;
	case 2:
		if((col & 0x01) == 0) key = '4'; 
		else if((col & 0x02) == 0) key = '5'; 
		else if((col & 0x04) == 0) key = '6'; 
		else if((col & 0x08) == 0) key = 'D'; 
		break;
	case 3:
		if((col & 0x01) == 0) key = '1'; 
		else if((col & 0x02) == 0) key = '2'; 
		else if((col & 0x04) == 0) key = '3'; 
		else if((col & 0x08) == 0) key = 'C'; 
		break;
	default:
		break;
	}
	
	return key;
}
/********************************************************************
// Function name	: gprs_get_key
// Description	    : ����м����·��ؼ������򷵻�0
// Return type		: char
// Argument         : 
*********************************************************************/
char gprs_get_key()
{
	char ch = 0;
	
	return Uart_GetKey();
//	if(gprs_key_recv_write == gprs_key_recv_read)
//	{
//		/* no key found */
//		ch = 0;
//	}else
//	{
//		ch = gprs_key_recv_buf[gprs_key_recv_read];
//		gprs_key_recv_read ++;
//		if(gprs_key_recv_read >= GPRS_RECV_CMD_MAX_BUF)
//			gprs_key_recv_read = 0;
//	}
	return ch;
}

int  timer1_count = 0;
enum KEYBOARD_SCAN_STATUS
{
	KEYBOARD_SCAN_FIRST,
	KEYBOARD_SCAN_SECOND,
	KEYBOARD_SCAN_THIRD,
	KEYBOARD_SCAN_FOURTH
};
int row = 0;
unsigned char output_0x02000000 = 0xff;
unsigned char 	ascii_key, input_key[4], input_key1[4], key_mask = 0x0F;
unsigned char*	keyboard_port_scan = (unsigned char*)0x10000000;
unsigned char*	keyboard_port_value = (unsigned char*)0x10000002;
int              keyboard_scan_status[4] = {
														KEYBOARD_SCAN_FIRST,
														KEYBOARD_SCAN_FIRST,
														KEYBOARD_SCAN_FIRST,
														KEYBOARD_SCAN_FIRST
													  };
void timer1_isr1() __attribute__ ((interrupt("IRQ")));
/********************************************************************
// Function name	: timer1_isr
// Description	    : ��ʱ��1�жϷ����������ɨ����̣�ÿ��10msһ���ж�
// Return type		: void
// Argument         : void
*********************************************************************/
void timer1_isr1(void)
{
	int loopcnt = row, bexit = 0;
	int temp;
	
	// ���TIMER1�ж�
	ClearPending(BIT_TIMER1);
	timer1_count++;
	
	// 20ms
	for( loopcnt = row; loopcnt < row + 4; loopcnt ++)
	{
		if(loopcnt >= 4)
			temp = loopcnt - 4;
		else
			temp = loopcnt;
		switch(keyboard_scan_status[temp])
		{
			case KEYBOARD_SCAN_FIRST:
				*keyboard_port_scan = output_0x02000000 & (~(0x00000001<<temp));        /*��row���õ͵�ƽ	*/
				keyboard_scan_status[temp] = KEYBOARD_SCAN_SECOND;
				bexit = 1;
				break;
			case KEYBOARD_SCAN_SECOND:
				input_key[temp] = (*keyboard_port_value) & key_mask;	/*����ȡ��һ��ɨ��ֵ*/
				if(input_key[temp] == key_mask)	
					keyboard_scan_status[temp] = KEYBOARD_SCAN_FIRST;		/* û�а���,�ص���ʼ״̬			*/
				else
				{
					keyboard_scan_status[temp] = KEYBOARD_SCAN_THIRD;		/* �а���		*/
					bexit = 1;
				}
				break;
			case KEYBOARD_SCAN_THIRD:
				if (((*keyboard_port_value) & key_mask) != input_key[temp]) 
					keyboard_scan_status[temp] = KEYBOARD_SCAN_FIRST;	
				else	
				{
					ascii_key = key_get_char(temp, input_key[temp]);
					keyboard_scan_status[temp] = KEYBOARD_SCAN_FOURTH;
					
					*keyboard_port_scan = output_0x02000000 & (~(0x00000001<<temp));        /*��row���õ͵�ƽ	*/
					bexit = 1;
				}
				break;
			case KEYBOARD_SCAN_FOURTH:
				input_key1[temp] = (*keyboard_port_value) & key_mask;	/*����ȡ��һ��ɨ��ֵ*/
				if(input_key1[temp] == key_mask)	
				{
					// get a key
					gprs_recv_key(ascii_key);					
					keyboard_scan_status[temp] = KEYBOARD_SCAN_FIRST;
				}else
				{
					*keyboard_port_scan = output_0x02000000 & (~(0x00000001<<temp));        /*��row���õ͵�ƽ	*/
					bexit = 1;
				}
				break;
		}			
		if(bexit)
			break;
	}
	
	row = temp;
}
/********************************************************************
// Function name	: timer_init
// Description	    : ��ʼ����ʱ��
// Return type		: void
// Argument         : void
*********************************************************************/
void timer_init(void)
{
	// ��ʱ����ʼ��
    Irq_Request(IRQ_TIMER1, timer1_isr1);

    rTCFG0 = rTCFG0 & ~(0xffffff) | 0x000f0f;         //Dead zone=0,Prescaler1=15(0x0f),Prescaler0=15(0x0f)
    rTCFG1 = rTCFG1 & ~(0xffffff) | 0x001233;         //All interrupt,Mux4=1/2,Mux3=1/4,Mux2=1/8,Mux1=1/16,Mux0=1/16

    //Timer input clock frequency = PCLK/(prescaler value+1)/(divider value)
    rTCNTB1 = 0x7aa;           //(1/(50MHz/16/16)) * 0x7aa (1962) = 0.01s ( Hz)
    
    rTCON  = rTCON & ~(0xffffff) | 0xa00;         //Auto reload, Inverter off, Manual update, Dead zone disable, Stop  
    
    rTCON  = rTCON & ~(0xffffff) | 0x900;         //Auto reload(T0=One-shot),Inverter off,No operation,Dead zone disable,Start
	
	Irq_Enable(IRQ_TIMER1);
}
/********************************************************************
// Function name	: gprs_tel_call_in
// Description	    : GPRS�绰���빦�ܲ���
// Return type		: void
// Argument         : 
*********************************************************************/
void gprs_tel_call_in()
{
	enum GPRS_TEL_IN_STATUS
	{
		GPRS_TEL_CALL_IN_IDLE,     // �ȴ�����
		GPRS_TEL_CALL_IN_ANSWER,   // �绰Ӧ��
		GPRS_TEL_CALL_IN_PHONE_ON, // ��ʼͨ��
	};

	int gprs_tel_call_in_status;
	int bexit = 0;
	char gprs_cmd_send_string[512];
	char gprs_cmd_recv_string[512];
	int  gprs_recv_msg_code;
	int key;

	gprs_tel_call_in_status = GPRS_TEL_CALL_IN_IDLE;
	while(bexit == 0)
	{
		key = gprs_get_key();
		switch(key)
		{
			case 0x0a:
			case 0x0d:
			case 'F':
			if(gprs_tel_call_in_status == GPRS_TEL_CALL_IN_IDLE)
			{
				// �����绰
				TRACE("����\n");
				strcpy(gprs_cmd_send_string, "ATA\r");
				gprs_send_cmd(gprs_cmd_send_string);
				gprs_tel_call_in_status = GPRS_TEL_CALL_IN_ANSWER;
			}
			break;
			case 'E':
			if(gprs_tel_call_in_status == GPRS_TEL_CALL_IN_ANSWER || \
			   gprs_tel_call_in_status == GPRS_TEL_CALL_IN_PHONE_ON)
			{
				// ����ͨ�����һ�
				TRACE("�һ�\n");
				strcpy(gprs_cmd_send_string, "ATH\r");
				gprs_send_cmd(gprs_cmd_send_string);
				gprs_tel_call_in_status = GPRS_TEL_CALL_IN_IDLE;
				bexit = 1;
			}else
			{
				// �˳�
				gprs_tel_call_in_status = GPRS_TEL_CALL_IN_IDLE;
				bexit = 1;
			}
			break;
		}

		// ��������
		gprs_recv_cmd(gprs_cmd_recv_string);
		gprs_recv_msg_code = gprs_analyze_msg(gprs_cmd_recv_string);
		if(gprs_tel_call_in_status == GPRS_TEL_CALL_IN_ANSWER)
		{
			switch(gprs_recv_msg_code)
			{
				// ����
				case AT_RECV_MSG_OK:
					strcpy(gprs_cmd_send_string, "AT+PPSPKR=0\r");
					Delay(200);
					gprs_send_cmd(gprs_cmd_send_string);
					Delay(200);
					gprs_send_cmd(gprs_cmd_send_string);
					gprs_tel_call_in_status = GPRS_TEL_CALL_IN_PHONE_ON;
					TRACE("��ʼͨ��\n");
				break;				
			}
		}
	}

	TRACE("�˳����빦��\n");
}
/********************************************************************
// Function name	: gprs_tel_call_out
// Description	    : �绰�������Գ���
// Return type		: void
// Argument         : 
*********************************************************************/
void gprs_tel_call_out()
{
	enum GPRS_TEL_OUT_STATUS
	{
		GPRS_TEL_CALL_OUT_IDLE,     // �ȴ�����
		GPRS_TEL_CALL_OUT_GET_NUM,  // ��ʼ�������
		GPRS_TEL_CALL_OUT_CALLING,  // ��������
		GPRS_TEL_CALL_OUT_PHONE_ON, // ��ʼͨ��
	};

	int gprs_tel_call_out_status;
	int bexit = 0;
	int key;
	char strcallnum[20];
	char gprs_cmd_send_string[512];
	char gprs_cmd_recv_string[512];
	char strtemp[10];
	strcallnum[0] = 0;
	int  gprs_recv_msg_code;
	
	gprs_tel_call_out_status = GPRS_TEL_CALL_OUT_IDLE;
	while(bexit == 0)
	{
		key = gprs_get_key();
		switch(key)
		{
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '0':
				if(gprs_tel_call_out_status == GPRS_TEL_CALL_OUT_IDLE || \
				   gprs_tel_call_out_status == GPRS_TEL_CALL_OUT_GET_NUM)
				{
					gprs_tel_call_out_status = GPRS_TEL_CALL_OUT_GET_NUM;
					sprintf(strtemp, "%c", key);
					TRACE(strtemp);
					strcat(strcallnum, strtemp);
				}
				break;
			case 'E':
				if(gprs_tel_call_out_status == GPRS_TEL_CALL_OUT_CALLING || \
				   gprs_tel_call_out_status == GPRS_TEL_CALL_OUT_PHONE_ON)
				{
					// ����ͨ�����һ�
					TRACE("�һ�\n�������Ż���Cancel�˳�\n");
					strcpy(gprs_cmd_send_string, "ATH\r");
					gprs_send_cmd(gprs_cmd_send_string);
					gprs_tel_call_out_status = GPRS_TEL_CALL_OUT_IDLE;
					// ��������
					strcallnum[0] = 0;
				}else
				{
					// �˳�
					gprs_tel_call_out_status = GPRS_TEL_CALL_OUT_IDLE;
					bexit = 1;
				}
				break;
			case 'F':
			case 0x0a:
			case 0x0d:
				TRACE("\n��ʼ���Ż���Cancel�һ�\n");
				gprs_tel_call_out_status = GPRS_TEL_CALL_OUT_CALLING;
				strcpy(gprs_cmd_send_string, "ATD");
				strcat(gprs_cmd_send_string, strcallnum);
				strcat(gprs_cmd_send_string, ";\r");
				gprs_send_cmd(gprs_cmd_send_string);
				break;
		}
		
		gprs_recv_cmd(gprs_cmd_recv_string);
		gprs_recv_msg_code = gprs_analyze_msg(gprs_cmd_recv_string);
		if(gprs_tel_call_out_status == GPRS_TEL_CALL_OUT_CALLING)
		{
			switch(gprs_recv_msg_code)
			{
				// ����
				case AT_RECV_MSG_OK:
					TRACE("��ʼͨ��\n");
					strcpy(gprs_cmd_send_string, "AT+PPSPKR=0\r");
					Delay(200);
					gprs_send_cmd(gprs_cmd_send_string);
					Delay(200);
					gprs_send_cmd(gprs_cmd_send_string);
					gprs_tel_call_out_status = GPRS_TEL_CALL_OUT_PHONE_ON;
				break;				
				// �Է��һ�
				case AT_RECV_MSG_NO_CARRIER:
				// ����
				case AT_RECV_MSG_ERROR:
					gprs_tel_call_out_status = GPRS_TEL_CALL_OUT_IDLE;
				break;
			}
		}
	}
	
	TRACE("�˳���������\n");
}
/********************************************************************
// Function name	: TestTel
// Description	    : �绰����
// Return type		: void
// Argument         : 
*********************************************************************/
void TestTel()
{
	enum GPRS_TEL_STATUS
	{
		GPRS_TEL_INIT,     // ���ڳ�ʼ��
		GPRS_TEL_IDLE,     // ��ʼ����ϣ��ȴ�����
		GPRS_TEL_CALL_IN,  // ���� 
		GPRS_TEL_CALL_OUT, // ����
		GPRS_TEL_OUT       // �˳�
	};
	
	char gprs_cmd_send_string[512];
	char gprs_cmd_recv_string[512];
	char ch;
	int  loopcnt;
	int  gprs_tel_status;
	int  bexit = 0;

	// ��ӡ��ʾ��Ϣ	
	TRACE("�绰����!\n");
	TRACE("���������У���ȴ�...\n");
	
	// ϵͳ��ʼ��
	gprs_tel_status = GPRS_TEL_INIT;
	gprs_init();	
////////////////////////////////////////////////////////////////
// GPRS��λ	
{
	int loopcnt;
__RESET:
	// ��ʱһ��ʱ��
	Delay(150);
	
	// ����AT����
	gprs_send_cmd("\r");
	gprs_send_cmd("AT\r");
	for(loopcnt = 0; loopcnt < 30; loopcnt++)
	{
		// ��ȡ����������ȡ��OK����Ϊ��λ�ɹ����������¸�λ
		gprs_recv_cmd(gprs_cmd_recv_string);
		if(strstr(gprs_cmd_recv_string, "OK") != 0)
			break;
		Delay(10);
	}
	if(loopcnt == 30)
	{
		Delay(300);
		gprs_pwr_on_off(GPRS_PWR_ON);
		goto __RESET;
	}
}
///////////////////////////////////////////////////////////////

	Delay(1500);
	gprs_send_cmd("\r");	
	gprs_send_cmd("ATE0\r");
	
	// �������״̬������״̬��
	gprs_tel_status = GPRS_TEL_IDLE;		
	// ��ʾ������	
	TRACE("1 - ���� Cancel�˳�\n");
	while(bexit == 0)
	{
		int   key;
		switch(gprs_tel_status)
		{
			// ����״̬���ȴ�����
			case GPRS_TEL_IDLE:
				key = gprs_get_key();
				if(key != 0)
				{
					switch(key)
					{
						case '1':
							gprs_tel_status = GPRS_TEL_CALL_OUT;
							break;
						case '2':
							gprs_tel_status = GPRS_TEL_CALL_IN;
							break;
						case 'E':
							gprs_tel_status = GPRS_TEL_OUT;
							bexit = 1;
							break;
					}
				}
				
				// ��������
				gprs_recv_cmd(gprs_cmd_recv_string);
				if(gprs_analyze_msg(gprs_cmd_recv_string) == AT_RECV_MSG_RING)
				{
					// �绰����
					gprs_tel_status = GPRS_TEL_CALL_IN;
				}
			break;
			// �绰����
			case GPRS_TEL_CALL_IN:
				// ��ʾ�绰����˵�
				TRACE("�绰���룬Enter������\n");
				gprs_tel_call_in();
				gprs_tel_status = GPRS_TEL_IDLE;
			break;
			// �绰����
			case GPRS_TEL_CALL_OUT:
				// ��ʾ�绰�����˵�
				TRACE("���е绰���벦��\n");
				gprs_tel_call_out();
				TRACE("1 - ���� Cancel�˳�\n");
				gprs_tel_status = GPRS_TEL_IDLE;
			break;
		}
	}	

	// ����
	TRACE("�˳��绰����\n");		
	gprs_uninit();
}
/********************************************************************
// Function name	: TestSMS
// Description	    : ���Ź��ܲ���
// Return type		: void
// Argument         : 
*********************************************************************/
void TestSMS()
{
	enum GPRS_SMS_STATUS
	{
		GPRS_SMS_INIT,     // ���ڳ�ʼ��
		GPRS_SMS_IDLE,     // ��ʼ����ϣ��ȴ�����
		GPRS_SMS_GET_NUM,  // ������� 
		GPRS_SMS_SEND,     // ����
	};
	
	char strcallnum[20];
	char gprs_cmd_send_string[512];
	char gprs_cmd_recv_string[512];
	char strtemp[10];
	int  gprs_recv_msg_code;
	char ch;
	int  loopcnt;
	int  gprs_sms_status;
	int  bexit = 0;
	strcallnum[0] = 0;
    
    // ��ӡ��ʾ��Ϣ	
	TRACE("���Ź���!\n");
	TRACE("���������У���ȴ�...\n");
	
	// ϵͳ��ʼ��
	gprs_sms_status = GPRS_SMS_INIT;
	gprs_init();	

////////////////////////////////////////////////////////////////
// GPRS��λ	
{
	int loopcnt;
__RESET:
	// ��ʱһ��ʱ��
	Delay(150);
	
	// ����AT����
	gprs_send_cmd("\r");
	gprs_send_cmd("AT\r");
	
	for(loopcnt = 0; loopcnt < 30; loopcnt++)
	{
		// ��ȡ����������ȡ��OK����Ϊ��λ�ɹ����������¸�λ
		gprs_recv_cmd(gprs_cmd_recv_string);
		if(strstr(gprs_cmd_recv_string, "OK") != 0)
			break;
		Delay(10);
	}
	if(loopcnt == 30)
	{
		Delay(300);
		gprs_pwr_on_off(GPRS_PWR_ON);
		goto __RESET;
	}
}
///////////////////////////////////////////////////////////////

	Delay(1500);
	gprs_send_cmd("\r");	
	gprs_send_cmd("ATE0\r");
	
	// switch to pdu mode
	strcpy(gprs_cmd_send_string, "AT+CMGF=0\r");
	gprs_send_cmd(gprs_cmd_send_string);

	strcpy(gprs_cmd_send_string, "AT+CNMI=2,1\r");
	gprs_send_cmd(gprs_cmd_send_string);
	
	// �������״̬������״̬��
	gprs_sms_status = GPRS_SMS_IDLE;
	// ��ʾ������	
	TRACE("������룬ENTER�����ͣ�Cancel�˳�\n");
	
	while(bexit == 0)
	{
		int   key;
		int   length;
		key = gprs_get_key();
		switch(key)
		{
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '0':
				if(gprs_sms_status == GPRS_SMS_IDLE || \
				   gprs_sms_status == GPRS_SMS_GET_NUM)
				{
					gprs_sms_status = GPRS_SMS_GET_NUM;
					sprintf(strtemp, "%c", key);
					TRACE(strtemp);
					strcat(strcallnum, strtemp);
				}
				break;
			case 'E':
				// �˳�
				gprs_sms_status = GPRS_SMS_IDLE;
				bexit = 1;
				break;
			case 'F':
			case 0x0d:
			case 0x0a:
				TRACE("���Ͷ���Ϣ\n");
				gprs_sms_status = GPRS_SMS_SEND;
				// ���÷������ĺ���
				strcpy(gprs_cmd_send_string, "AT+CSCA=\"+8613800270500\"\r");
				gprs_send_cmd(gprs_cmd_send_string);
				loopcnt = 0;
				do
				{
					gprs_recv_cmd(gprs_cmd_recv_string);
					gprs_recv_msg_code = gprs_analyze_msg(gprs_cmd_recv_string);
					
					loopcnt ++;
					if(loopcnt >= 20)
					{
						break;
					}
				}while(gprs_recv_msg_code != AT_RECV_MSG_OK && gprs_recv_msg_code != AT_RECV_MSG_ERROR);
				
				if(gprs_recv_msg_code == AT_RECV_MSG_ERROR)
				{
					TRACE("�������ĺ������ô���\n");
					bexit = 1;
					break;
				}else
				{
					// ���Ͷ���
					SM_PARAM Src;
				
					strcpy(Src.SCA, "8613800270500");			// ����Ϣ�������ĺ���(SMSC��ַ)
					strcpy(Src.TPA, "86");
					strcat(Src.TPA, strcallnum);			// Ŀ������ظ�����(TP-DA��TP-RA)
					strcallnum[0] = 0;
					Src.TP_PID = 0;			// �û���ϢЭ���ʶ(TP-PID)
					Src.TP_DCS = 8;			// �û���Ϣ���뷽ʽ(TP-DCS)
					strcpy(Src.TP_SCTS, "04060308421002");		// ����ʱ����ַ���(TP_SCTS), ����ʱ�õ�
					strcpy(Src.TP_UD, "�人��ά����Ϣ�������޹�˾��ӭ��\r\nwww.cvtech.com.cn");		// ԭʼ�û���Ϣ(����ǰ�������TP-UD)
					
					TRACE("��ʼ����\n");
					
					gprsSendMessage(&Src);
					
					TRACE("�������\n");
					TRACE("������룬ENTER�����ͣ�Cancel�˳�\n");
					gprs_sms_status = GPRS_SMS_IDLE;
				}
				break;
		}
		
		gprs_recv_cmd(gprs_cmd_recv_string);
		gprs_recv_msg_code = gprs_analyze_msg(gprs_cmd_recv_string);
		if(gprs_recv_msg_code == AT_RECV_MSG_CMTI)
		{
			char *pDest;
			
			// �յ�����Ϣ
			TRACE("�յ�����Ϣ\n");
			
			// ��������Ϣ
			pDest = strstr(gprs_cmd_recv_string, ",");
			if(pDest != 0)
			{
				SM_PARAM Msg;
				
				pDest++;
				// �Ķ�����Ϣ
				TRACE("�Ķ�����Ϣ\n");
				
				// Read Message
				sprintf(gprs_cmd_send_string, "AT+CMGR=%d\r", atoi(pDest));
				gprs_send_cmd(gprs_cmd_send_string);
				// receive message
				do
				{
					gprs_recv_cmd(gprs_cmd_recv_string);
					gprs_recv_msg_code = gprs_analyze_msg(gprs_cmd_recv_string);
					if(gprs_recv_msg_code == AT_RECV_MSG_CMGR)
					{
						SM_PARAM Msg;
						
						gprs_recv_cmd(gprs_cmd_recv_string);
						gprs_recv_cmd(gprs_cmd_recv_string);
						gprsDecodePdu(gprs_cmd_recv_string, &Msg);
						
						gprs_print_msg(&Msg);
						break;
					}
				}while(1);
			}
		}
	}
	TRACE("�˳�\n");
}
/********************************************************************
// Function name	: gprs_print_msg
// Description	    : ��ӡ����Ϣ
// Return type		: void
// Argument         : SM_PARAM* pMsg
*********************************************************************/
void gprs_print_msg(SM_PARAM* pMsg)
{
	char tmp[100];
	
	TRACE("\n��������:");
	TRACE(pMsg->SCA);
	TRACE("\n����:");
	TRACE(pMsg->TPA);
	TRACE("\nʱ��:");
	sprintf(tmp, "20%c%c��%c%c��%c%c��%c%cʱ%c%c��%c%c��", pMsg->TP_SCTS[0], pMsg->TP_SCTS[1], \
	                               pMsg->TP_SCTS[2], pMsg->TP_SCTS[3], \
	                               pMsg->TP_SCTS[4], pMsg->TP_SCTS[5], \
	                               pMsg->TP_SCTS[6], pMsg->TP_SCTS[7], \
	                               pMsg->TP_SCTS[8], pMsg->TP_SCTS[9], \
	                               pMsg->TP_SCTS[10], pMsg->TP_SCTS[11]);
	TRACE(tmp);
	TRACE("\n����:");
	TRACE(pMsg->TP_UD);
	TRACE("\n");
}
/********************************************************************
// Function name	: Main
// Description	    : ������
// Return type		: void
// Argument         : void
*********************************************************************/
void Main(void)
{
	/* ����ϵͳʱ�� */
    ChangeClockDivider(2,1);
    unsigned int mpll_val = 0 ;
    mpll_val = (92<<12)|(1<<4)|(1);
    ChangeMPllValue((mpll_val>>12)&0xff, (mpll_val>>4)&0x3f, mpll_val&3); 
	
	/* �жϳ�ʼ�� */
    Isr_Init();
    /* ��ʼ���˿� */
    Port_Init();
    
    /* ��ʼ������ */
    Uart_Init(0,115200);
    Uart_Select(0);
    
#if 1
    TestTel();
#else
	TestSMS();
#endif
	
	while(1)
	{
	}
}
