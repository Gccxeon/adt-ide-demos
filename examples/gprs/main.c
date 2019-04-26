/****************************************************************************/
/*                                                                          */
/* FILE NAME                                      VERSION                   */
/*                                                                          */
/* GPRS.C                                            1.0                    */
/*                                                                          */
/* DESCRIPTION                                                              */
/*                                                                          */
/*     JX44B0(S3C44B0X)GPRS通讯实验                                         */
/*                                                                          */
/*                                                                          */
/* DATA STRUCTURES                                                          */
/*                                                                          */
/* FUNCTIONS :                                                              */
/*     在JX44B0教学实验箱进行GPRS通讯实验                                   */
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
/* 学习JX44B0中电话呼叫功能和中英文短信功能的实现方法：                     */
/* 注意：                                                                   */
/*     1. 该实验仅仅适用与JX44B0-2以及JX44B0-3实验箱                        */
/*     2. 实验之前请阅读用户手册，并进行正确的硬件连接                      */
/*     3. 实验过程需要SIM卡，SIM可以使用中国移动和中国联通的各种手机卡      */
/*     4. SIM卡请不要带电插拔，否则容易导致烧卡                             */
/*     5. 短信实验中需要修改短信中心号码，请参照您的手机中的设置设置该值，  */
/*     注意去掉前面的'+'号                                                  */
/****************************************************************************/

/* 包含文件 */
#include "2410addr.h"
#include "2410lib.h"
#include "gprs.h"

/* 按键缓冲区 */
char gprs_key_recv_buf[GPRS_RECV_CMD_MAX_BUF];
int  gprs_key_recv_read = 0;
int  gprs_key_recv_write = 0;

/********************************************************************
// Function name	: TRACE
// Description	    : 在串口0上打印调试信息
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
// Description	    : 将获取的键值加入按键缓冲区
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
		// 缓冲区以满
		gprs_key_recv_read ++;
		if(gprs_key_recv_read >= GPRS_RECV_CMD_MAX_BUF)
			gprs_key_recv_read = 0;
	}
}
/********************************************************************
// Function name	: key_get_char
// Description	    : 键盘扫描码
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
// Description	    : 如果有键按下返回键，否则返回0
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
// Description	    : 定时器1中断服务程序，用于扫描键盘，每隔10ms一次中断
// Return type		: void
// Argument         : void
*********************************************************************/
void timer1_isr1(void)
{
	int loopcnt = row, bexit = 0;
	int temp;
	
	// 清除TIMER1中断
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
				*keyboard_port_scan = output_0x02000000 & (~(0x00000001<<temp));        /*将row列置低电平	*/
				keyboard_scan_status[temp] = KEYBOARD_SCAN_SECOND;
				bexit = 1;
				break;
			case KEYBOARD_SCAN_SECOND:
				input_key[temp] = (*keyboard_port_value) & key_mask;	/*并获取第一次扫描值*/
				if(input_key[temp] == key_mask)	
					keyboard_scan_status[temp] = KEYBOARD_SCAN_FIRST;		/* 没有按键,回到开始状态			*/
				else
				{
					keyboard_scan_status[temp] = KEYBOARD_SCAN_THIRD;		/* 有按键		*/
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
					
					*keyboard_port_scan = output_0x02000000 & (~(0x00000001<<temp));        /*将row列置低电平	*/
					bexit = 1;
				}
				break;
			case KEYBOARD_SCAN_FOURTH:
				input_key1[temp] = (*keyboard_port_value) & key_mask;	/*并获取第一次扫描值*/
				if(input_key1[temp] == key_mask)	
				{
					// get a key
					gprs_recv_key(ascii_key);					
					keyboard_scan_status[temp] = KEYBOARD_SCAN_FIRST;
				}else
				{
					*keyboard_port_scan = output_0x02000000 & (~(0x00000001<<temp));        /*将row列置低电平	*/
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
// Description	    : 初始化定时器
// Return type		: void
// Argument         : void
*********************************************************************/
void timer_init(void)
{
	// 定时器初始化
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
// Description	    : GPRS电话呼入功能测试
// Return type		: void
// Argument         : 
*********************************************************************/
void gprs_tel_call_in()
{
	enum GPRS_TEL_IN_STATUS
	{
		GPRS_TEL_CALL_IN_IDLE,     // 等待输入
		GPRS_TEL_CALL_IN_ANSWER,   // 电话应答
		GPRS_TEL_CALL_IN_PHONE_ON, // 开始通话
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
				// 接听电话
				TRACE("接听\n");
				strcpy(gprs_cmd_send_string, "ATA\r");
				gprs_send_cmd(gprs_cmd_send_string);
				gprs_tel_call_in_status = GPRS_TEL_CALL_IN_ANSWER;
			}
			break;
			case 'E':
			if(gprs_tel_call_in_status == GPRS_TEL_CALL_IN_ANSWER || \
			   gprs_tel_call_in_status == GPRS_TEL_CALL_IN_PHONE_ON)
			{
				// 正在通话，挂机
				TRACE("挂机\n");
				strcpy(gprs_cmd_send_string, "ATH\r");
				gprs_send_cmd(gprs_cmd_send_string);
				gprs_tel_call_in_status = GPRS_TEL_CALL_IN_IDLE;
				bexit = 1;
			}else
			{
				// 退出
				gprs_tel_call_in_status = GPRS_TEL_CALL_IN_IDLE;
				bexit = 1;
			}
			break;
		}

		// 接收输入
		gprs_recv_cmd(gprs_cmd_recv_string);
		gprs_recv_msg_code = gprs_analyze_msg(gprs_cmd_recv_string);
		if(gprs_tel_call_in_status == GPRS_TEL_CALL_IN_ANSWER)
		{
			switch(gprs_recv_msg_code)
			{
				// 接听
				case AT_RECV_MSG_OK:
					strcpy(gprs_cmd_send_string, "AT+PPSPKR=0\r");
					Delay(200);
					gprs_send_cmd(gprs_cmd_send_string);
					Delay(200);
					gprs_send_cmd(gprs_cmd_send_string);
					gprs_tel_call_in_status = GPRS_TEL_CALL_IN_PHONE_ON;
					TRACE("开始通话\n");
				break;				
			}
		}
	}

	TRACE("退出呼入功能\n");
}
/********************************************************************
// Function name	: gprs_tel_call_out
// Description	    : 电话呼出测试程序
// Return type		: void
// Argument         : 
*********************************************************************/
void gprs_tel_call_out()
{
	enum GPRS_TEL_OUT_STATUS
	{
		GPRS_TEL_CALL_OUT_IDLE,     // 等待输入
		GPRS_TEL_CALL_OUT_GET_NUM,  // 开始输入号码
		GPRS_TEL_CALL_OUT_CALLING,  // 呼出号码
		GPRS_TEL_CALL_OUT_PHONE_ON, // 开始通话
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
					// 正在通话，挂机
					TRACE("挂机\n继续拨号或者Cancel退出\n");
					strcpy(gprs_cmd_send_string, "ATH\r");
					gprs_send_cmd(gprs_cmd_send_string);
					gprs_tel_call_out_status = GPRS_TEL_CALL_OUT_IDLE;
					// 号码清零
					strcallnum[0] = 0;
				}else
				{
					// 退出
					gprs_tel_call_out_status = GPRS_TEL_CALL_OUT_IDLE;
					bexit = 1;
				}
				break;
			case 'F':
			case 0x0a:
			case 0x0d:
				TRACE("\n开始拨号或者Cancel挂机\n");
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
				// 接听
				case AT_RECV_MSG_OK:
					TRACE("开始通话\n");
					strcpy(gprs_cmd_send_string, "AT+PPSPKR=0\r");
					Delay(200);
					gprs_send_cmd(gprs_cmd_send_string);
					Delay(200);
					gprs_send_cmd(gprs_cmd_send_string);
					gprs_tel_call_out_status = GPRS_TEL_CALL_OUT_PHONE_ON;
				break;				
				// 对方挂机
				case AT_RECV_MSG_NO_CARRIER:
				// 错误
				case AT_RECV_MSG_ERROR:
					gprs_tel_call_out_status = GPRS_TEL_CALL_OUT_IDLE;
				break;
			}
		}
	}
	
	TRACE("退出呼出功能\n");
}
/********************************************************************
// Function name	: TestTel
// Description	    : 电话功能
// Return type		: void
// Argument         : 
*********************************************************************/
void TestTel()
{
	enum GPRS_TEL_STATUS
	{
		GPRS_TEL_INIT,     // 正在初始化
		GPRS_TEL_IDLE,     // 初始化完毕，等待输入
		GPRS_TEL_CALL_IN,  // 呼入 
		GPRS_TEL_CALL_OUT, // 呼出
		GPRS_TEL_OUT       // 退出
	};
	
	char gprs_cmd_send_string[512];
	char gprs_cmd_recv_string[512];
	char ch;
	int  loopcnt;
	int  gprs_tel_status;
	int  bexit = 0;

	// 打印提示信息	
	TRACE("电话功能!\n");
	TRACE("网络连接中，请等待...\n");
	
	// 系统初始化
	gprs_tel_status = GPRS_TEL_INIT;
	gprs_init();	
////////////////////////////////////////////////////////////////
// GPRS复位	
{
	int loopcnt;
__RESET:
	// 延时一定时间
	Delay(150);
	
	// 发送AT命令
	gprs_send_cmd("\r");
	gprs_send_cmd("AT\r");
	for(loopcnt = 0; loopcnt < 30; loopcnt++)
	{
		// 获取结果，如果读取到OK，认为复位成功，否则重新复位
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
	
	// 进入空闲状态，开启状态机
	gprs_tel_status = GPRS_TEL_IDLE;		
	// 显示主界面	
	TRACE("1 - 拨号 Cancel退出\n");
	while(bexit == 0)
	{
		int   key;
		switch(gprs_tel_status)
		{
			// 空闲状态，等待输入
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
				
				// 接收输入
				gprs_recv_cmd(gprs_cmd_recv_string);
				if(gprs_analyze_msg(gprs_cmd_recv_string) == AT_RECV_MSG_RING)
				{
					// 电话呼入
					gprs_tel_status = GPRS_TEL_CALL_IN;
				}
			break;
			// 电话呼入
			case GPRS_TEL_CALL_IN:
				// 显示电话呼入菜单
				TRACE("电话呼入，Enter键接听\n");
				gprs_tel_call_in();
				gprs_tel_status = GPRS_TEL_IDLE;
			break;
			// 电话呼出
			case GPRS_TEL_CALL_OUT:
				// 显示电话呼出菜单
				TRACE("呼叫电话，请拨号\n");
				gprs_tel_call_out();
				TRACE("1 - 拨号 Cancel退出\n");
				gprs_tel_status = GPRS_TEL_IDLE;
			break;
		}
	}	

	// 结束
	TRACE("退出电话功能\n");		
	gprs_uninit();
}
/********************************************************************
// Function name	: TestSMS
// Description	    : 短信功能测试
// Return type		: void
// Argument         : 
*********************************************************************/
void TestSMS()
{
	enum GPRS_SMS_STATUS
	{
		GPRS_SMS_INIT,     // 正在初始化
		GPRS_SMS_IDLE,     // 初始化完毕，等待输入
		GPRS_SMS_GET_NUM,  // 输入号码 
		GPRS_SMS_SEND,     // 呼出
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
    
    // 打印提示信息	
	TRACE("短信功能!\n");
	TRACE("网络连接中，请等待...\n");
	
	// 系统初始化
	gprs_sms_status = GPRS_SMS_INIT;
	gprs_init();	

////////////////////////////////////////////////////////////////
// GPRS复位	
{
	int loopcnt;
__RESET:
	// 延时一定时间
	Delay(150);
	
	// 发送AT命令
	gprs_send_cmd("\r");
	gprs_send_cmd("AT\r");
	
	for(loopcnt = 0; loopcnt < 30; loopcnt++)
	{
		// 获取结果，如果读取到OK，认为复位成功，否则重新复位
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
	
	// 进入空闲状态，开启状态机
	gprs_sms_status = GPRS_SMS_IDLE;
	// 显示主界面	
	TRACE("输入号码，ENTER键发送，Cancel退出\n");
	
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
				// 退出
				gprs_sms_status = GPRS_SMS_IDLE;
				bexit = 1;
				break;
			case 'F':
			case 0x0d:
			case 0x0a:
				TRACE("发送短消息\n");
				gprs_sms_status = GPRS_SMS_SEND;
				// 设置服务中心号码
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
					TRACE("服务中心号码设置错误\n");
					bexit = 1;
					break;
				}else
				{
					// 发送短信
					SM_PARAM Src;
				
					strcpy(Src.SCA, "8613800270500");			// 短消息服务中心号码(SMSC地址)
					strcpy(Src.TPA, "86");
					strcat(Src.TPA, strcallnum);			// 目标号码或回复号码(TP-DA或TP-RA)
					strcallnum[0] = 0;
					Src.TP_PID = 0;			// 用户信息协议标识(TP-PID)
					Src.TP_DCS = 8;			// 用户信息编码方式(TP-DCS)
					strcpy(Src.TP_SCTS, "04060308421002");		// 服务时间戳字符串(TP_SCTS), 接收时用到
					strcpy(Src.TP_UD, "武汉创维特信息技术有限公司欢迎您\r\nwww.cvtech.com.cn");		// 原始用户信息(编码前或解码后的TP-UD)
					
					TRACE("开始发送\n");
					
					gprsSendMessage(&Src);
					
					TRACE("发送完毕\n");
					TRACE("输入号码，ENTER键发送，Cancel退出\n");
					gprs_sms_status = GPRS_SMS_IDLE;
				}
				break;
		}
		
		gprs_recv_cmd(gprs_cmd_recv_string);
		gprs_recv_msg_code = gprs_analyze_msg(gprs_cmd_recv_string);
		if(gprs_recv_msg_code == AT_RECV_MSG_CMTI)
		{
			char *pDest;
			
			// 收到短消息
			TRACE("收到短消息\n");
			
			// 解析短消息
			pDest = strstr(gprs_cmd_recv_string, ",");
			if(pDest != 0)
			{
				SM_PARAM Msg;
				
				pDest++;
				// 阅读短消息
				TRACE("阅读短消息\n");
				
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
	TRACE("退出\n");
}
/********************************************************************
// Function name	: gprs_print_msg
// Description	    : 打印短消息
// Return type		: void
// Argument         : SM_PARAM* pMsg
*********************************************************************/
void gprs_print_msg(SM_PARAM* pMsg)
{
	char tmp[100];
	
	TRACE("\n服务中心:");
	TRACE(pMsg->SCA);
	TRACE("\n来自:");
	TRACE(pMsg->TPA);
	TRACE("\n时间:");
	sprintf(tmp, "20%c%c年%c%c月%c%c日%c%c时%c%c分%c%c秒", pMsg->TP_SCTS[0], pMsg->TP_SCTS[1], \
	                               pMsg->TP_SCTS[2], pMsg->TP_SCTS[3], \
	                               pMsg->TP_SCTS[4], pMsg->TP_SCTS[5], \
	                               pMsg->TP_SCTS[6], pMsg->TP_SCTS[7], \
	                               pMsg->TP_SCTS[8], pMsg->TP_SCTS[9], \
	                               pMsg->TP_SCTS[10], pMsg->TP_SCTS[11]);
	TRACE(tmp);
	TRACE("\n内容:");
	TRACE(pMsg->TP_UD);
	TRACE("\n");
}
/********************************************************************
// Function name	: Main
// Description	    : 主函数
// Return type		: void
// Argument         : void
*********************************************************************/
void Main(void)
{
	/* 配置系统时钟 */
    ChangeClockDivider(2,1);
    unsigned int mpll_val = 0 ;
    mpll_val = (92<<12)|(1<<4)|(1);
    ChangeMPllValue((mpll_val>>12)&0xff, (mpll_val>>4)&0x3f, mpll_val&3); 
	
	/* 中断初始化 */
    Isr_Init();
    /* 初始化端口 */
    Port_Init();
    
    /* 初始化串口 */
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
