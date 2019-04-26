#include "2410addr.h"
#include "2410lib.h"
#include "def.h"
#include "gprs.h"
#include "gb2312-unicode.h"
#include "uart0.h"

void Uart1_RxInt() __attribute__ ((interrupt("IRQ")));

int gprs_ctrl_value = 0x0;
int gprs_ctrl_value1 = 0x0;

/* 接收数据缓冲区 */
char gprs_recv_buf[GPRS_RECV_CMD_MAX_BUF];
int  gprs_recv_read = 0;
int  gprs_recv_write = 0;

int int_bak;
/********************************************************************
// Function name	: gprs_disable_int
// Description	    : 关中断
// Return type		: void
// Argument         : 
*********************************************************************/
void gprs_disable_int()
{
	SET_IF();
}
/********************************************************************
// Function name	: gprs_enable_int
// Description	    : 开中断
// Return type		: void
// Argument         : 
*********************************************************************/
void gprs_enable_int()
{
	CLR_IF();
}	
/********************************************************************
// Function name	: gprs_uart_ctrl
// Description	    : GPRS使用串口1，串口0用于显示，GPRS初始化之前请
//                    调用该函数进行初始化
// Return type		: void
// Argument         : int uart = 0x01
*********************************************************************/
void gprs_uart_ctrl(int uart)
{
	
	gprs_ctrl_value &= ~GPRS_CONTROL_MASK_UART;
	gprs_ctrl_value |= uart;
	
	gprs_ctrl_value1 |= 0x20;
	
	*(unsigned char *)GPRS_CONTROL_ADDR = gprs_ctrl_value;
	*(unsigned char *)0x28000006 = gprs_ctrl_value1;
}
/********************************************************************
// Function name	: gprs_send_cmd
// Description	    : 发送GPRS命令字串
// Return type		: void
// Argument         : char *cmdstring
*********************************************************************/
void gprs_send_cmd(char *cmdstring)
{
	// disable int
	gprs_disable_int();
	
	// select uart 1
	Uart_Select(0);
	// send command
	Uart_Printf("Send -> %s\n", cmdstring);

	// select uart 1
	Uart_Select(1);
	// send command
	Uart_Printf(cmdstring);

	// enable int
	gprs_enable_int();
}
/********************************************************************
// Function name	: gprs_recv_char
// Description	    : 从GPRS接收字符，只能从中断服务函数中调用
// Return type		: void
// Argument         : 
*********************************************************************/
void gprs_recv_char()
{
	char ch;

	do
	{	
		// disable int
//		gprs_disable_int();
	
		// receive command
		Uart_Select(1);
		ch = Uart_GetKey();
	
		// enable int
//		gprs_enable_int();
		if(ch == 0)
		{
			return;
		}else
		{
			// select uart 1
			gprs_recv_buf[gprs_recv_write] = ch;
			gprs_recv_write ++;
			if(gprs_recv_write >= GPRS_RECV_CMD_MAX_BUF)
				gprs_recv_write = 0;
			
			if(gprs_recv_write == gprs_recv_read)
			{
				// 缓冲区以满
				gprs_recv_read ++;
				if(gprs_recv_read >= GPRS_RECV_CMD_MAX_BUF)
					gprs_recv_read = 0;
			}
		}
	}while(1);
}
/********************************************************************
// Function name	: gprs_recv_cmd
// Description	    : 
// Return type		: int: 
//                      GPRS_OK -- 接收到命令
//                      GPRS_ERR -- 未接收到命令
// Argument         : char *cmd:返回的命令
*********************************************************************/
int gprs_recv_cmd(char *cmd)
{
	int loopcnt = 0;
	int ncount = 0;
	
	while(1)
	{	
		if(gprs_recv_read == gprs_recv_write)
		{
				return GPRS_ERR;
			ncount ++;
			if(ncount >= 1000)
			{
				cmd[loopcnt++] = 0;
				return GPRS_ERR;
			}
			continue;
		}
		ncount = 0;
		if( (gprs_recv_buf[gprs_recv_read] == '\r') || (gprs_recv_buf[gprs_recv_read] == '\n'))
		{
			cmd[loopcnt ++] = 0;
			gprs_recv_read ++;
			if(gprs_recv_read >= GPRS_RECV_CMD_MAX_BUF)
				gprs_recv_read = 0;
			
			if(strlen(cmd))
			{
				TRACE("\nRecv <- ");
				TRACE(cmd);
				TRACE("\n");
			}
			return GPRS_OK;
		}else
		{
			cmd[loopcnt ++] = gprs_recv_buf[gprs_recv_read];
			gprs_recv_read ++;
			if(gprs_recv_read >= GPRS_RECV_CMD_MAX_BUF)
				gprs_recv_read = 0;
		}
	}
}
/********************************************************************
// Function name	: Uart1_RxInt
// Description	    : 串口1接收中断服务程序
// Return type		: void
// Argument         : void
*********************************************************************/
void Uart1_RxInt(void)
{
    rINTSUBMSK|=(BIT_SUB_RXD1|BIT_SUB_TXD1|BIT_SUB_ERR1);
    if(rSUBSRCPND&BIT_SUB_RXD1) gprs_recv_char();
    else __sub_Uart1_RxErrInt();

    ClearPending(BIT_UART1);
    rSUBSRCPND=(BIT_SUB_RXD1|BIT_SUB_ERR1);	// Clear Sub int pending    
    rINTSUBMSK&=~(BIT_SUB_RXD1|BIT_SUB_ERR1);    
}
/********************************************************************
// Function name	: gprs_init
// Description	    : GPRS初始化函数
// Return type		: void
// Argument         : 
*********************************************************************/
void gprs_init()
{
	// 串口初始化
	Uart_Init(0,115200);
	
	pISR_UART1=(unsigned)Uart1_RxInt;
	
	// uart0 -> db9  uart1 -> gprs
	gprs_uart_ctrl(0x1);

	// GPC10 -> Power On/Off
	// GPRS模块电源控制
/*	rGPGCON |= (1 << 20);
	rGPGCON &= ~(1 << 21);
*/
	// 打开UART TX中断
    rUCON1 &= 0x400;	// For the PCLK <-> UCLK fuction
    rUCON1 |= (TX_INTTYPE<<9)|(RX_INTTYPE<<8)|(0<<7)|(1<<6)|(0<<5)|(0<<4)|(1<<2)|(1);

    rUCON1 |= (TX_INTTYPE<<9)|(RX_INTTYPE<<8)|(1<<7)|(1<<6)|(0<<5)|(0<<4)|(1<<2)|(1);
    rUFCON1 |= (3 << 4) | (1<< 1) | 1;
    //Clock,Tx:pulse,Rx:pulse,Rx timeout:x,Rx error int:o,Loop-back:x,Send break:x,Tx:int,Rx:int

    ClearPending(BIT_UART1);
    rINTMSK=~(BIT_UART1);
    rSUBSRCPND=(BIT_SUB_RXD1|BIT_SUB_ERR1);
    rINTSUBMSK=~(BIT_SUB_RXD1|BIT_SUB_ERR1);
}
/********************************************************************
// Function name	: gprs_uninit
// Description	    : gprs模块退出
// Return type		: void
// Argument         : 
*********************************************************************/
void gprs_uninit()
{
	// disable int
	gprs_disable_int();
	
	Uart_Select(0);
	
	// enable int
	gprs_enable_int();

	gprs_ctrl_value1 &= ~0x20;
	
	*(unsigned char *)0x28000006 = gprs_ctrl_value1;
}
/********************************************************************
// Function name	: gprs_pwr_on_off
// Description	    : GPRS复位操作
// Return type		: void
// Argument         : int bon
//                        GPRS_PWR_ON   开启
//                        GPRS_PWR_OFF 0   关闭
*********************************************************************/
void gprs_pwr_on_off(int bon)
{
	static status_pwr = GPRS_PWR_OFF;
	int    loopcnt;
	
	if(bon == GPRS_PWR_ON)
	{
		if(status_pwr == GPRS_PWR_ON)
			return;
		rGPGCON |= (1 << 20);
		rGPGCON &= ~(1 << 21);
		rGPGDAT |= (1 << 10 );
		Delay(100);
		rGPGDAT &= ~(1 << 10 );
		Delay(1500);
		rGPGDAT |= (1 << 10 );
		
		status_pwr = GPRS_PWR_ON;
		
		Delay(1000);
	}else
	{
		if(status_pwr == GPRS_PWR_OFF)
			return;
		rGPGCON |= (1 << 20);
		rGPGCON &= ~(1 << 21);
		rGPGDAT |= (1 << 10 );
		Delay(100);
		rGPGDAT &= ~(1 << 10 );
		Delay(1500);
		rGPGDAT |= (1 << 10 );
		
		status_pwr = GPRS_PWR_OFF;
	}
}
/********************************************************************
// Function name	: gprs_analyze_msg
// Description	    : 分析接收到的GPRS命令
// Return type		: int
// Argument         : char * message
*********************************************************************/
int gprs_analyze_msg(char * message)
{
	if (strstr(message, "OK") != 0)
	{
		if (strstr(message, "+CMGS:") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_SMS_SEND_RESPONSE, 0, 0);
		}
		else if (strstr(message, "+CMGR:") != 0)
		{
			return AT_RECV_MSG_CMGR;
		}
		else if (strstr(message, "+CSQ:") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_SIGNAL_STRENGTH, 0, 0);
		}
		else if (strstr(message, "+CBC:") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_BATTERY_LEVEL, 0, 0);
		}
		else if (strstr(message, "+CRSL:") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_RINGER_LEVEL, 0, 0);
		}
		else if (strstr(message, "+CREG:") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_NETWORK_REGISTRATION, 0, 0);
		}
		else if (strstr(message, "+CGMI") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_MANUFACTURER_ID, 0, 0);
		}
		else if (strstr(message, "+CGMM") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_MODEL_ID, 0, 0);
		}
		else if (strstr(message, "+CGMR") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_REVISION_ID, 0, 0);
		}
		else if (strstr(message, "+CNUM") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_SUBSCRIBER_NUMBER, 0, 0);
		}
		else if (strstr(message, "+CSCS") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_TE_CHARACTER_SET, 0, 0);
		}
		else if (strstr(message, "+CCLK") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_TIME, 0, 0);
		}
		else if (strstr(message, "+CRC") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_CELLULAR_RESULT_CODE, 0, 0);
		}
		else if (strstr(message, "+CR") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_REPORTING_CONTROL, 0, 0);
		}
		else if (strstr(message, "S0") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_S0, 0, 0);
		}
		else if (strstr(message, "S7") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_S7, 0, 0);
		}
		else if (strstr(message, "S8") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_S8, 0, 0);
		}
		else if (strstr(message, "S10") != 0)
		{
			// threadData.m_rmReceMsg.m_strMessage1 = message;
			// ::PostMessage(threadData.m_hWnd, WM_S10, 0, 0);
		}
		else
		{
			return AT_RECV_MSG_OK;
		}
	}
	else if (strstr(message, "+CMGR:") != 0)
	{
		return AT_RECV_MSG_CMGR;
	}
	else if (strstr(message, "RING") != 0)
	{
		// 振铃
		return AT_RECV_MSG_RING;
	}
	else if (strstr(message, "NO CARRIER") != 0)
	{
		// 无载波
		return AT_RECV_MSG_NO_CARRIER;
	}
	else if (strstr(message, "ERROR") != 0)
	{
		// 错误
		return AT_RECV_MSG_ERROR;
	}
	else if (strstr(message, "CMTI") != 0)
	{
		// NEW SMS
		return AT_RECV_MSG_CMTI;
	}else if (strstr(message, ">") != 0)
	{
		return AT_RECV_GET_MSG;
	}
	
	return AT_RECV_MSG_NULL;
}
// 多字节字符转换为宽字符
// 返回: 宽字符数据长度
int MultiByteToWideChar(
            unsigned int CodePage,
            unsigned int dwFlags,
            char * lpMultiByteStr,
            int cbMultiByte,
            unsigned short * lpWideCharStr,
            int cchWideChar)
{
    int retval =0;
    char * lpCurrentUnixLCType = 0;

    if ( 0 == CodePage )
    {
        unsigned int nIndex = 0;

        if ( cbMultiByte == -1)
        {
            cbMultiByte = strlen(lpMultiByteStr) + 1;           
        }

        if (cchWideChar == 0)
        {
            retval = cbMultiByte;
            goto EXIT;
        }

        if ( cbMultiByte > cchWideChar )  
        {
            retval = 0;
            goto EXIT;
        }

        for (nIndex=0; nIndex < cbMultiByte; nIndex++ )
        {
            int i;
	        if ((lpMultiByteStr[nIndex] < 0x80))
	        {
	        	lpWideCharStr[retval] = lpMultiByteStr[nIndex];
	          	retval+=1;
	        }
	        else
	        {
	        	i = 0;
	            lpWideCharStr[retval] = '?';
	            	
	            while(1)
	            {
	            	if(gb2312_unicode_tran_table[i].gb2312==0 && gb2312_unicode_tran_table[i].unicode==0)
	            		break;
	            	if(gb2312_unicode_tran_table[i].gb2312 == (((lpMultiByteStr[nIndex] << 8) + lpMultiByteStr[nIndex + 1]) - 0x8080))
	            	{
	            		lpWideCharStr[retval] = gb2312_unicode_tran_table[i].unicode;
	            		retval++;
	            		nIndex++;
	            		break;
	            	}
	            	i ++;
	            }
	        }
        }
        
        goto EXIT;    
    }
         
EXIT:
    return retval;
}
//  wcslen
int wcslen(const short *string)
{
    int nChar = 0;
  
    if ( !string )
    {
        return 0;
    }
    while (*string++)
    {
        nChar++;
    }

    return nChar;
}
// 宽字符转换为多字符
int WideCharToMultiByte(
            unsigned int CodePage,
            unsigned int dwFlags,
            unsigned short * lpWideCharStr,
            int cchWideChar,
            char * lpMultiByteStr,
            int cbMultiByte,
            char * lpDefaultChar,
            int * lpUsedDefaultChar)
{
    int retval =0;
    char * lpCurrentUnixLCType = 0;

	if (0 == CodePage)
    {
        unsigned int nIndex = 0;

        if ( cchWideChar == -1)
        {
            cchWideChar = wcslen(lpWideCharStr) + 1; 
        }

        if (cbMultiByte == 0)
        {
            /* cbMultiByte is 0, we must return the lenght of 
              the destination buffer in bytes */
            retval = cchWideChar; 
            goto EXIT;
        }

        if ( cchWideChar > cbMultiByte )  
        {
            retval = 0;
            goto EXIT;
        }

        /* perform a reverse lookup on the PAL_CP_1252 table */
        for (nIndex=0 ; nIndex < cchWideChar; nIndex++ )
        {  
            int i;
            if ((lpWideCharStr[nIndex] < 0x80))
            {
                  lpMultiByteStr[retval] = (unsigned char) lpWideCharStr[nIndex];
          		  retval++;
            }
            else
            {
            	i = 0;
            	lpMultiByteStr[retval] = '?';
            	
            	while(1)
            	{
            		if(gb2312_unicode_tran_table[i].gb2312==0 && gb2312_unicode_tran_table[i].unicode==0)
            			break;
            		if(gb2312_unicode_tran_table[i].unicode == lpWideCharStr[nIndex])
            		{
            			lpMultiByteStr[retval] = ((gb2312_unicode_tran_table[i].gb2312 + 0x8080) & 0xff00) >> 8;
            			retval++;
            			lpMultiByteStr[retval] = (gb2312_unicode_tran_table[i].gb2312 + 0x8080) & 0xff;
            			retval++;
            			break;
            		}
            		i ++;
            	}
            }         
        }
         
        goto EXIT;    
    } 
    
EXIT:
    return retval;
}
// 可打印字符串转换为字节数据
// 如："C8329BFD0E01" --> {0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01}
// pSrc: 源字符串指针
// pDst: 目标数据指针
// nSrcLength: 源字符串长度
// 返回: 目标数据长度
int gprsString2Bytes(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	int i;
	for(i=0; i<nSrcLength; i+=2)
	{
		// 输出高4位
		if(*pSrc>='0' && *pSrc<='9')
		{
			*pDst = (*pSrc - '0') << 4;
		}
		else
		{
			*pDst = (*pSrc - 'A' + 10) << 4;
		}

		pSrc++;

		// 输出低4位
		if(*pSrc>='0' && *pSrc<='9')
		{
			*pDst |= *pSrc - '0';
		}
		else
		{
			*pDst |= *pSrc - 'A' + 10;
		}

		pSrc++;
		pDst++;
	}

	// 返回目标数据长度
	return nSrcLength / 2;
}
// 字节数据转换为可打印字符串
// 如：{0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01} --> "C8329BFD0E01" 
// pSrc: 源数据指针
// pDst: 目标字符串指针
// nSrcLength: 源数据长度
// 返回: 目标字符串长度
int gprsBytes2String(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	const char tab[]="0123456789ABCDEF";	// 0x0-0xf的字符查找表
	int i;

	for(i=0; i<nSrcLength; i++)
	{
		*pDst++ = tab[*pSrc >> 4];		// 输出低4位
		*pDst++ = tab[*pSrc & 0x0f];	// 输出高4位
		pSrc++;
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标字符串长度
	return nSrcLength * 2;
}
// 7bit编码
// pSrc: 源字符串指针
// pDst: 目标编码串指针
// nSrcLength: 源字符串长度
// 返回: 目标编码串长度
int gprsEncode7bit(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	int nSrc;		// 源字符串的计数值
	int nDst;		// 目标编码串的计数值
	int nChar;		// 当前正在处理的组内字符字节的序号，范围是0-7
	unsigned char nLeft;	// 上一字节残余的数据

	// 计数值初始化
	nSrc = 0;
	nDst = 0;

	// 将源串每8个字节分为一组，压缩成7个字节
	// 循环该处理过程，直至源串被处理完
	// 如果分组不到8字节，也能正确处理
	while(nSrc<nSrcLength)
	{
		// 取源字符串的计数值的最低3位
		nChar = nSrc & 7;

		// 处理源串的每个字节
		if(nChar == 0)
		{
			// 组内第一个字节，只是保存起来，待处理下一个字节时使用
			nLeft = *pSrc;
		}
		else
		{
			// 组内其它字节，将其右边部分与残余数据相加，得到一个目标编码字节
			*pDst = (*pSrc << (8-nChar)) | nLeft;

			// 将该字节剩下的左边部分，作为残余数据保存起来
			nLeft = *pSrc >> nChar;

			// 修改目标串的指针和计数值
			pDst++;
			nDst++;
		}

		// 修改源串的指针和计数值
		pSrc++;
		nSrc++;
	}

	// 返回目标串长度
	return nDst;
}
// 7bit解码
// pSrc: 源编码串指针
// pDst: 目标字符串指针
// nSrcLength: 源编码串长度
// 返回: 目标字符串长度
int gprsDecode7bit(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	int nSrc;		// 源字符串的计数值
	int nDst;		// 目标解码串的计数值
	int nByte;		// 当前正在处理的组内字节的序号，范围是0-6
	unsigned char nLeft;	// 上一字节残余的数据

	// 计数值初始化
	nSrc = 0;
	nDst = 0;
	
	// 组内字节序号和残余数据初始化
	nByte = 0;
	nLeft = 0;

	// 将源数据每7个字节分为一组，解压缩成8个字节
	// 循环该处理过程，直至源数据被处理完
	// 如果分组不到7字节，也能正确处理
	while(nSrc<nSrcLength)
	{
		// 将源字节右边部分与残余数据相加，去掉最高位，得到一个目标解码字节
		*pDst = ((*pSrc << nByte) | nLeft) & 0x7f;

		// 将该字节剩下的左边部分，作为残余数据保存起来
		nLeft = *pSrc >> (7-nByte);

		// 修改目标串的指针和计数值
		pDst++;
		nDst++;

		// 修改字节计数值
		nByte++;

		// 到了一组的最后一个字节
		if(nByte == 7)
		{
			// 额外得到一个目标解码字节
			*pDst = nLeft;

			// 修改目标串的指针和计数值
			pDst++;
			nDst++;

			// 组内字节序号和残余数据初始化
			nByte = 0;
			nLeft = 0;
		}

		// 修改源串的指针和计数值
		pSrc++;
		nSrc++;
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标串长度
	return nDst;
}
// 8bit编码
// pSrc: 源字符串指针
// pDst: 目标编码串指针
// nSrcLength: 源字符串长度
// 返回: 目标编码串长度
int gprsEncode8bit(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	// 简单复制
	memcpy(pDst, pSrc, nSrcLength);

	return nSrcLength;
}
// 8bit解码
// pSrc: 源编码串指针
// pDst: 目标字符串指针
// nSrcLength: 源编码串长度
// 返回: 目标字符串长度
int gprsDecode8bit(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	// 简单复制
	memcpy(pDst, pSrc, nSrcLength);

	// 输出字符串加个结束符
	*pDst = '\0';

	return nSrcLength;
}
// UCS2编码
// pSrc: 源字符串指针
// pDst: 目标编码串指针
// nSrcLength: 源字符串长度
// 返回: 目标编码串长度
int gprsEncodeUcs2(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	int nDstLength;		// UNICODE宽字符数目
	WCHAR wchar[128];	// UNICODE串缓冲区
	int i;

	// 字符串-->UNICODE串
	nDstLength = MultiByteToWideChar(0, 0, pSrc, nSrcLength, wchar, 128);

	// 高低字节对调，输出
	for(i=0; i<nDstLength; i++)
	{
		*pDst++ = wchar[i] >> 8;		// 先输出高位字节
		*pDst++ = wchar[i] & 0xff;		// 后输出低位字节
	}

	// 返回目标编码串长度
	return nDstLength * 2;
}
// UCS2解码
// pSrc: 源编码串指针
// pDst: 目标字符串指针
// nSrcLength: 源编码串长度
// 返回: 目标字符串长度
int gprsDecodeUcs2(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;		// UNICODE宽字符数目
	WCHAR wchar[128];	// UNICODE串缓冲区
	int i;

	// 高低字节对调，拼成UNICODE
	memcpy(wchar, pSrc, nSrcLength);
	for(i=0; i<nSrcLength/2; i++)
	{
		wchar[i] = *pSrc++ << 8;	// 先高位字节
		wchar[i] |= *pSrc++;		// 后低位字节
	}

	// UNICODE串-->字符串
	nDstLength = WideCharToMultiByte(0, 0, wchar, nSrcLength/2, pDst, 160, 0, 0);

	// 输出字符串加个结束符
	pDst[nDstLength] = '\0';

	// 返回目标字符串长度
	return nDstLength;
}
// 正常顺序的字符串转换为两两颠倒的字符串，若长度为奇数，补'F'凑成偶数
// 如："8613851872468" --> "683158812764F8"
// pSrc: 源字符串指针
// pDst: 目标字符串指针
// nSrcLength: 源字符串长度
// 返回: 目标字符串长度
int gprsInvertNumbers(const char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;		// 目标字符串长度
	char ch;			// 用于保存一个字符
	int   i;

	// 复制串长度
	nDstLength = nSrcLength;

	// 两两颠倒
	for(i=0; i<nSrcLength;i+=2)
	{
		ch = *pSrc++;		// 保存先出现的字符
		*pDst++ = *pSrc++;	// 复制后出现的字符
		*pDst++ = ch;		// 复制先出现的字符
	}

	// 源串长度是奇数吗？
	if(nSrcLength & 1)
	{
		*(pDst-2) = 'F';	// 补'F'
		nDstLength++;		// 目标串长度加1
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标字符串长度
	return nDstLength;
}
// 两两颠倒的字符串转换为正常顺序的字符串
// 如："683158812764F8" --> "8613851872468"
// pSrc: 源字符串指针
// pDst: 目标字符串指针
// nSrcLength: 源字符串长度
// 返回: 目标字符串长度
int gprsSerializeNumbers(const char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;		// 目标字符串长度
	char ch;			// 用于保存一个字符
	int  i;

	// 复制串长度
	nDstLength = nSrcLength;

	// 两两颠倒
	for(i=0; i<nSrcLength;i+=2)
	{
		ch = *pSrc++;		// 保存先出现的字符
		*pDst++ = *pSrc++;	// 复制后出现的字符
		*pDst++ = ch;		// 复制先出现的字符
	}

	// 最后的字符是'F'吗？
	if(*(pDst-1) == 'F')
	{
		pDst--;
		nDstLength--;		// 目标字符串长度减1
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标字符串长度
	return nDstLength;
}
// PDU编码，用于编制、发送短消息
// pSrc: 源PDU参数指针
// pDst: 目标PDU串指针
// 返回: 目标PDU串长度
int gprsEncodePdu(const SM_PARAM* pSrc, char* pDst)
{
	int nLength;			// 内部用的串长度
	int nDstLength;			// 目标PDU串长度
	unsigned char buf[256];	// 内部用的缓冲区

	// SMSC地址信息段
	nLength = strlen(pSrc->SCA);	// SMSC地址字符串的长度	
	buf[0] = (char)((nLength & 1) == 0 ? nLength : nLength + 1) / 2 + 1;	// SMSC地址信息长度
	buf[1] = 0x91;		// 固定: 用国际格式号码
	nDstLength = gprsBytes2String(buf, pDst, 2);		// 转换2个字节到目标PDU串
	nDstLength += gprsInvertNumbers(pSrc->SCA, &pDst[nDstLength], nLength);	// 转换SMSC号码到目标PDU串

	// TPDU段基本参数、目标地址等
	nLength = strlen(pSrc->TPA);	// TP-DA地址字符串的长度
	buf[0] = 0x11;					// 是发送短信(TP-MTI=01)，TP-VP用相对格式(TP-VPF=10)
	buf[1] = 0;						// TP-MR=0
	buf[2] = (char)nLength;			// 目标地址数字个数(TP-DA地址字符串真实长度)
	buf[3] = 0x91;					// 固定: 用国际格式号码
	nDstLength += gprsBytes2String(buf, &pDst[nDstLength], 4);		// 转换4个字节到目标PDU串
	nDstLength += gprsInvertNumbers(pSrc->TPA, &pDst[nDstLength], nLength);	// 转换TP-DA到目标PDU串

	// TPDU段协议标识、编码方式、用户信息等
	nLength = strlen(pSrc->TP_UD);	// 用户信息字符串的长度
	buf[0] = pSrc->TP_PID;			// 协议标识(TP-PID)
	buf[1] = pSrc->TP_DCS;			// 用户信息编码方式(TP-DCS)
	buf[2] = 0;						// 有效期(TP-VP)为5分钟
	if(pSrc->TP_DCS == GSM_7BIT)	
	{
		// 7-bit编码方式
		buf[3] = nLength;			// 编码前长度
		nLength = gprsEncode7bit(pSrc->TP_UD, &buf[4], nLength+1) + 4;	// 转换TP-DA到目标PDU串
	}
	else if(pSrc->TP_DCS == GSM_UCS2)
	{
	
		// UCS2编码方式
		buf[3] = gprsEncodeUcs2(pSrc->TP_UD, &buf[4], nLength);	// 转换TP-DA到目标PDU串
		nLength = buf[3] + 4;		// nLength等于该段数据长度
	}
	else
	{
		// 8-bit编码方式
		buf[3] = gprsEncode8bit(pSrc->TP_UD, &buf[4], nLength);	// 转换TP-DA到目标PDU串
		nLength = buf[3] + 4;		// nLength等于该段数据长度
	}
	nDstLength += gprsBytes2String(buf, &pDst[nDstLength], nLength);		// 转换该段数据到目标PDU串

	// 返回目标字符串长度
	return nDstLength;
}
// PDU解码，用于接收、阅读短消息
// pSrc: 源PDU串指针
// pDst: 目标PDU参数指针
// 返回: 用户信息串长度
int gprsDecodePdu(const char* pSrc, SM_PARAM* pDst)
{
	int nDstLength;			// 目标PDU串长度
	unsigned char tmp;		// 内部用的临时字节变量
	unsigned char buf[256];	// 内部用的缓冲区

	// SMSC地址信息段
	gprsString2Bytes(pSrc, &tmp, 2);	// 取长度
	tmp = (tmp - 1) * 2;	// SMSC号码串长度
	pSrc += 4;			// 指针后移，忽略了SMSC地址格式
	gprsSerializeNumbers(pSrc, pDst->SCA, tmp);	// 转换SMSC号码到目标PDU串
	pSrc += tmp;		// 指针后移

	// TPDU段基本参数、回复地址等
	gprsString2Bytes(pSrc, &tmp, 2);	// 取基本参数
	pSrc += 2;		// 指针后移
//	if(tmp & 0x80)
	{
		// 包含回复地址，取回复地址信息
		gprsString2Bytes(pSrc, &tmp, 2);	// 取长度
		if(tmp & 1) tmp += 1;	// 调整奇偶性
		pSrc += 4;			// 指针后移，忽略了回复地址(TP-RA)格式
		gprsSerializeNumbers(pSrc, pDst->TPA, tmp);	// 取TP-RA号码
		pSrc += tmp;		// 指针后移
	}

	// TPDU段协议标识、编码方式、用户信息等
	gprsString2Bytes(pSrc, (unsigned char*)&pDst->TP_PID, 2);	// 取协议标识(TP-PID)
	pSrc += 2;		// 指针后移
	gprsString2Bytes(pSrc, (unsigned char*)&pDst->TP_DCS, 2);	// 取编码方式(TP-DCS)
	pSrc += 2;		// 指针后移
	gprsSerializeNumbers(pSrc, pDst->TP_SCTS, 14);		// 服务时间戳字符串(TP_SCTS) 
	pSrc += 14;		// 指针后移
	gprsString2Bytes(pSrc, &tmp, 2);	// 用户信息长度(TP-UDL)
	pSrc += 2;		// 指针后移
	if(pDst->TP_DCS == GSM_7BIT)	
	{
		// 7-bit解码
		nDstLength = gprsString2Bytes(pSrc, buf, tmp & 7 ? (int)tmp * 7 / 4 + 2 : (int)tmp * 7 / 4);	// 格式转换
		gprsDecode7bit(buf, pDst->TP_UD, nDstLength);	// 转换到TP-DU
		nDstLength = tmp;
	}
	else if(pDst->TP_DCS == GSM_UCS2)
	{
		// UCS2解码
		nDstLength = gprsString2Bytes(pSrc, buf, tmp * 2);			// 格式转换
		nDstLength = gprsDecodeUcs2(buf, pDst->TP_UD, nDstLength);	// 转换到TP-DU
	}
	else
	{
		// 8-bit解码
		nDstLength = gprsString2Bytes(pSrc, buf, tmp * 2);			// 格式转换
		nDstLength = gprsDecode8bit(buf, pDst->TP_UD, nDstLength);	// 转换到TP-DU
	}

	// 返回目标字符串长度
	return nDstLength;
}
// 发送短消息
// pSrc: 源PDU参数指针
BOOL gprsSendMessage(const SM_PARAM* pSrc)
{
	int nPduLength;		// PDU串长度
	unsigned char nSmscLength;	// SMSC串长度
	int nResult;		// 
	char cmd[16];		// 命令串
	char temp[16];
	char pdu[512];		// PDU串
	char ans[128];		// 应答串

	nPduLength = gprsEncodePdu(pSrc, pdu);	// 根据PDU参数，编码PDU串
	pdu[nPduLength] = 0;

	gprsString2Bytes(pdu, &nSmscLength, 2);	// 取PDU串中的SMSC信息长度
	nSmscLength++;		// 加上长度字节本身

	// 命令中的长度，不包括SMSC信息长度，以数据字节计
	sprintf(cmd, "AT+CMGS=%d\r", nPduLength / 2 - nSmscLength);	// 生成命令

	gprs_send_cmd(cmd);	// 先输出命令串

	Delay(500);
	
	nResult = gprs_recv_cmd(ans);	// 读应答数据
	nResult = gprs_recv_cmd(ans);

	// 根据能否找到"\r\n> "决定成功与否
	if(nResult == GPRS_OK)
	{
		int loopcnt = 40;
		nResult = gprs_recv_cmd(ans);
		gprs_send_cmd(pdu);  // 得到肯定回答，继续输出PDU串

		pdu[0] = 0x1a; // CTRL + Z = 0x1a
		pdu[1] = 0x0;
		gprs_send_cmd(pdu);
		
		while(1)
		{
			Delay(10);
			gprs_recv_cmd(ans);	// 读应答数据

			// 根据能否找到"+CMS ERROR"决定成功与否
			if(strncmp(ans, "+CMS ERROR", 10) == 0)
			{
				TRACE("发送失败\n");
				return FALSE;
			}else if (strncmp(ans, "OK", 2) == 0)
			{
				TRACE("发送成功\n");
				return TRUE;
			}
			
			if(loopcnt-- == 0)
				return FALSE;
		}
	}
	TRACE("发送失败\n");
	gprs_recv_cmd(ans);
	gprs_recv_cmd(ans);
	return FALSE;
}
