#include "2410addr.h"
#include "2410lib.h"
#include "def.h"
#include "gprs.h"
#include "gb2312-unicode.h"
#include "uart0.h"

void Uart1_RxInt() __attribute__ ((interrupt("IRQ")));

int gprs_ctrl_value = 0x0;
int gprs_ctrl_value1 = 0x0;

/* �������ݻ����� */
char gprs_recv_buf[GPRS_RECV_CMD_MAX_BUF];
int  gprs_recv_read = 0;
int  gprs_recv_write = 0;

int int_bak;
/********************************************************************
// Function name	: gprs_disable_int
// Description	    : ���ж�
// Return type		: void
// Argument         : 
*********************************************************************/
void gprs_disable_int()
{
	SET_IF();
}
/********************************************************************
// Function name	: gprs_enable_int
// Description	    : ���ж�
// Return type		: void
// Argument         : 
*********************************************************************/
void gprs_enable_int()
{
	CLR_IF();
}	
/********************************************************************
// Function name	: gprs_uart_ctrl
// Description	    : GPRSʹ�ô���1������0������ʾ��GPRS��ʼ��֮ǰ��
//                    ���øú������г�ʼ��
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
// Description	    : ����GPRS�����ִ�
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
// Description	    : ��GPRS�����ַ���ֻ�ܴ��жϷ������е���
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
				// ����������
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
//                      GPRS_OK -- ���յ�����
//                      GPRS_ERR -- δ���յ�����
// Argument         : char *cmd:���ص�����
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
// Description	    : ����1�����жϷ������
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
// Description	    : GPRS��ʼ������
// Return type		: void
// Argument         : 
*********************************************************************/
void gprs_init()
{
	// ���ڳ�ʼ��
	Uart_Init(0,115200);
	
	pISR_UART1=(unsigned)Uart1_RxInt;
	
	// uart0 -> db9  uart1 -> gprs
	gprs_uart_ctrl(0x1);

	// GPC10 -> Power On/Off
	// GPRSģ���Դ����
/*	rGPGCON |= (1 << 20);
	rGPGCON &= ~(1 << 21);
*/
	// ��UART TX�ж�
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
// Description	    : gprsģ���˳�
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
// Description	    : GPRS��λ����
// Return type		: void
// Argument         : int bon
//                        GPRS_PWR_ON   ����
//                        GPRS_PWR_OFF 0   �ر�
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
// Description	    : �������յ���GPRS����
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
		// ����
		return AT_RECV_MSG_RING;
	}
	else if (strstr(message, "NO CARRIER") != 0)
	{
		// ���ز�
		return AT_RECV_MSG_NO_CARRIER;
	}
	else if (strstr(message, "ERROR") != 0)
	{
		// ����
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
// ���ֽ��ַ�ת��Ϊ���ַ�
// ����: ���ַ����ݳ���
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
// ���ַ�ת��Ϊ���ַ�
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
// �ɴ�ӡ�ַ���ת��Ϊ�ֽ�����
// �磺"C8329BFD0E01" --> {0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01}
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ������ָ��
// nSrcLength: Դ�ַ�������
// ����: Ŀ�����ݳ���
int gprsString2Bytes(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	int i;
	for(i=0; i<nSrcLength; i+=2)
	{
		// �����4λ
		if(*pSrc>='0' && *pSrc<='9')
		{
			*pDst = (*pSrc - '0') << 4;
		}
		else
		{
			*pDst = (*pSrc - 'A' + 10) << 4;
		}

		pSrc++;

		// �����4λ
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

	// ����Ŀ�����ݳ���
	return nSrcLength / 2;
}
// �ֽ�����ת��Ϊ�ɴ�ӡ�ַ���
// �磺{0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01} --> "C8329BFD0E01" 
// pSrc: Դ����ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ���ݳ���
// ����: Ŀ���ַ�������
int gprsBytes2String(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	const char tab[]="0123456789ABCDEF";	// 0x0-0xf���ַ����ұ�
	int i;

	for(i=0; i<nSrcLength; i++)
	{
		*pDst++ = tab[*pSrc >> 4];		// �����4λ
		*pDst++ = tab[*pSrc & 0x0f];	// �����4λ
		pSrc++;
	}

	// ����ַ����Ӹ�������
	*pDst = '\0';

	// ����Ŀ���ַ�������
	return nSrcLength * 2;
}
// 7bit����
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ����봮ָ��
// nSrcLength: Դ�ַ�������
// ����: Ŀ����봮����
int gprsEncode7bit(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	int nSrc;		// Դ�ַ����ļ���ֵ
	int nDst;		// Ŀ����봮�ļ���ֵ
	int nChar;		// ��ǰ���ڴ���������ַ��ֽڵ���ţ���Χ��0-7
	unsigned char nLeft;	// ��һ�ֽڲ��������

	// ����ֵ��ʼ��
	nSrc = 0;
	nDst = 0;

	// ��Դ��ÿ8���ֽڷ�Ϊһ�飬ѹ����7���ֽ�
	// ѭ���ô�����̣�ֱ��Դ����������
	// ������鲻��8�ֽڣ�Ҳ����ȷ����
	while(nSrc<nSrcLength)
	{
		// ȡԴ�ַ����ļ���ֵ�����3λ
		nChar = nSrc & 7;

		// ����Դ����ÿ���ֽ�
		if(nChar == 0)
		{
			// ���ڵ�һ���ֽڣ�ֻ�Ǳ�����������������һ���ֽ�ʱʹ��
			nLeft = *pSrc;
		}
		else
		{
			// ���������ֽڣ������ұ߲��������������ӣ��õ�һ��Ŀ������ֽ�
			*pDst = (*pSrc << (8-nChar)) | nLeft;

			// �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
			nLeft = *pSrc >> nChar;

			// �޸�Ŀ�괮��ָ��ͼ���ֵ
			pDst++;
			nDst++;
		}

		// �޸�Դ����ָ��ͼ���ֵ
		pSrc++;
		nSrc++;
	}

	// ����Ŀ�괮����
	return nDst;
}
// 7bit����
// pSrc: Դ���봮ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ���봮����
// ����: Ŀ���ַ�������
int gprsDecode7bit(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	int nSrc;		// Դ�ַ����ļ���ֵ
	int nDst;		// Ŀ����봮�ļ���ֵ
	int nByte;		// ��ǰ���ڴ���������ֽڵ���ţ���Χ��0-6
	unsigned char nLeft;	// ��һ�ֽڲ��������

	// ����ֵ��ʼ��
	nSrc = 0;
	nDst = 0;
	
	// �����ֽ���źͲ������ݳ�ʼ��
	nByte = 0;
	nLeft = 0;

	// ��Դ����ÿ7���ֽڷ�Ϊһ�飬��ѹ����8���ֽ�
	// ѭ���ô�����̣�ֱ��Դ���ݱ�������
	// ������鲻��7�ֽڣ�Ҳ����ȷ����
	while(nSrc<nSrcLength)
	{
		// ��Դ�ֽ��ұ߲��������������ӣ�ȥ�����λ���õ�һ��Ŀ������ֽ�
		*pDst = ((*pSrc << nByte) | nLeft) & 0x7f;

		// �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
		nLeft = *pSrc >> (7-nByte);

		// �޸�Ŀ�괮��ָ��ͼ���ֵ
		pDst++;
		nDst++;

		// �޸��ֽڼ���ֵ
		nByte++;

		// ����һ������һ���ֽ�
		if(nByte == 7)
		{
			// ����õ�һ��Ŀ������ֽ�
			*pDst = nLeft;

			// �޸�Ŀ�괮��ָ��ͼ���ֵ
			pDst++;
			nDst++;

			// �����ֽ���źͲ������ݳ�ʼ��
			nByte = 0;
			nLeft = 0;
		}

		// �޸�Դ����ָ��ͼ���ֵ
		pSrc++;
		nSrc++;
	}

	// ����ַ����Ӹ�������
	*pDst = '\0';

	// ����Ŀ�괮����
	return nDst;
}
// 8bit����
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ����봮ָ��
// nSrcLength: Դ�ַ�������
// ����: Ŀ����봮����
int gprsEncode8bit(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	// �򵥸���
	memcpy(pDst, pSrc, nSrcLength);

	return nSrcLength;
}
// 8bit����
// pSrc: Դ���봮ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ���봮����
// ����: Ŀ���ַ�������
int gprsDecode8bit(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	// �򵥸���
	memcpy(pDst, pSrc, nSrcLength);

	// ����ַ����Ӹ�������
	*pDst = '\0';

	return nSrcLength;
}
// UCS2����
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ����봮ָ��
// nSrcLength: Դ�ַ�������
// ����: Ŀ����봮����
int gprsEncodeUcs2(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	int nDstLength;		// UNICODE���ַ���Ŀ
	WCHAR wchar[128];	// UNICODE��������
	int i;

	// �ַ���-->UNICODE��
	nDstLength = MultiByteToWideChar(0, 0, pSrc, nSrcLength, wchar, 128);

	// �ߵ��ֽڶԵ������
	for(i=0; i<nDstLength; i++)
	{
		*pDst++ = wchar[i] >> 8;		// �������λ�ֽ�
		*pDst++ = wchar[i] & 0xff;		// �������λ�ֽ�
	}

	// ����Ŀ����봮����
	return nDstLength * 2;
}
// UCS2����
// pSrc: Դ���봮ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ���봮����
// ����: Ŀ���ַ�������
int gprsDecodeUcs2(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;		// UNICODE���ַ���Ŀ
	WCHAR wchar[128];	// UNICODE��������
	int i;

	// �ߵ��ֽڶԵ���ƴ��UNICODE
	memcpy(wchar, pSrc, nSrcLength);
	for(i=0; i<nSrcLength/2; i++)
	{
		wchar[i] = *pSrc++ << 8;	// �ȸ�λ�ֽ�
		wchar[i] |= *pSrc++;		// ���λ�ֽ�
	}

	// UNICODE��-->�ַ���
	nDstLength = WideCharToMultiByte(0, 0, wchar, nSrcLength/2, pDst, 160, 0, 0);

	// ����ַ����Ӹ�������
	pDst[nDstLength] = '\0';

	// ����Ŀ���ַ�������
	return nDstLength;
}
// ����˳����ַ���ת��Ϊ�����ߵ����ַ�����������Ϊ��������'F'�ճ�ż��
// �磺"8613851872468" --> "683158812764F8"
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ�ַ�������
// ����: Ŀ���ַ�������
int gprsInvertNumbers(const char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;		// Ŀ���ַ�������
	char ch;			// ���ڱ���һ���ַ�
	int   i;

	// ���ƴ�����
	nDstLength = nSrcLength;

	// �����ߵ�
	for(i=0; i<nSrcLength;i+=2)
	{
		ch = *pSrc++;		// �����ȳ��ֵ��ַ�
		*pDst++ = *pSrc++;	// ���ƺ���ֵ��ַ�
		*pDst++ = ch;		// �����ȳ��ֵ��ַ�
	}

	// Դ��������������
	if(nSrcLength & 1)
	{
		*(pDst-2) = 'F';	// ��'F'
		nDstLength++;		// Ŀ�괮���ȼ�1
	}

	// ����ַ����Ӹ�������
	*pDst = '\0';

	// ����Ŀ���ַ�������
	return nDstLength;
}
// �����ߵ����ַ���ת��Ϊ����˳����ַ���
// �磺"683158812764F8" --> "8613851872468"
// pSrc: Դ�ַ���ָ��
// pDst: Ŀ���ַ���ָ��
// nSrcLength: Դ�ַ�������
// ����: Ŀ���ַ�������
int gprsSerializeNumbers(const char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;		// Ŀ���ַ�������
	char ch;			// ���ڱ���һ���ַ�
	int  i;

	// ���ƴ�����
	nDstLength = nSrcLength;

	// �����ߵ�
	for(i=0; i<nSrcLength;i+=2)
	{
		ch = *pSrc++;		// �����ȳ��ֵ��ַ�
		*pDst++ = *pSrc++;	// ���ƺ���ֵ��ַ�
		*pDst++ = ch;		// �����ȳ��ֵ��ַ�
	}

	// �����ַ���'F'��
	if(*(pDst-1) == 'F')
	{
		pDst--;
		nDstLength--;		// Ŀ���ַ������ȼ�1
	}

	// ����ַ����Ӹ�������
	*pDst = '\0';

	// ����Ŀ���ַ�������
	return nDstLength;
}
// PDU���룬���ڱ��ơ����Ͷ���Ϣ
// pSrc: ԴPDU����ָ��
// pDst: Ŀ��PDU��ָ��
// ����: Ŀ��PDU������
int gprsEncodePdu(const SM_PARAM* pSrc, char* pDst)
{
	int nLength;			// �ڲ��õĴ�����
	int nDstLength;			// Ŀ��PDU������
	unsigned char buf[256];	// �ڲ��õĻ�����

	// SMSC��ַ��Ϣ��
	nLength = strlen(pSrc->SCA);	// SMSC��ַ�ַ����ĳ���	
	buf[0] = (char)((nLength & 1) == 0 ? nLength : nLength + 1) / 2 + 1;	// SMSC��ַ��Ϣ����
	buf[1] = 0x91;		// �̶�: �ù��ʸ�ʽ����
	nDstLength = gprsBytes2String(buf, pDst, 2);		// ת��2���ֽڵ�Ŀ��PDU��
	nDstLength += gprsInvertNumbers(pSrc->SCA, &pDst[nDstLength], nLength);	// ת��SMSC���뵽Ŀ��PDU��

	// TPDU�λ���������Ŀ���ַ��
	nLength = strlen(pSrc->TPA);	// TP-DA��ַ�ַ����ĳ���
	buf[0] = 0x11;					// �Ƿ��Ͷ���(TP-MTI=01)��TP-VP����Ը�ʽ(TP-VPF=10)
	buf[1] = 0;						// TP-MR=0
	buf[2] = (char)nLength;			// Ŀ���ַ���ָ���(TP-DA��ַ�ַ�����ʵ����)
	buf[3] = 0x91;					// �̶�: �ù��ʸ�ʽ����
	nDstLength += gprsBytes2String(buf, &pDst[nDstLength], 4);		// ת��4���ֽڵ�Ŀ��PDU��
	nDstLength += gprsInvertNumbers(pSrc->TPA, &pDst[nDstLength], nLength);	// ת��TP-DA��Ŀ��PDU��

	// TPDU��Э���ʶ�����뷽ʽ���û���Ϣ��
	nLength = strlen(pSrc->TP_UD);	// �û���Ϣ�ַ����ĳ���
	buf[0] = pSrc->TP_PID;			// Э���ʶ(TP-PID)
	buf[1] = pSrc->TP_DCS;			// �û���Ϣ���뷽ʽ(TP-DCS)
	buf[2] = 0;						// ��Ч��(TP-VP)Ϊ5����
	if(pSrc->TP_DCS == GSM_7BIT)	
	{
		// 7-bit���뷽ʽ
		buf[3] = nLength;			// ����ǰ����
		nLength = gprsEncode7bit(pSrc->TP_UD, &buf[4], nLength+1) + 4;	// ת��TP-DA��Ŀ��PDU��
	}
	else if(pSrc->TP_DCS == GSM_UCS2)
	{
	
		// UCS2���뷽ʽ
		buf[3] = gprsEncodeUcs2(pSrc->TP_UD, &buf[4], nLength);	// ת��TP-DA��Ŀ��PDU��
		nLength = buf[3] + 4;		// nLength���ڸö����ݳ���
	}
	else
	{
		// 8-bit���뷽ʽ
		buf[3] = gprsEncode8bit(pSrc->TP_UD, &buf[4], nLength);	// ת��TP-DA��Ŀ��PDU��
		nLength = buf[3] + 4;		// nLength���ڸö����ݳ���
	}
	nDstLength += gprsBytes2String(buf, &pDst[nDstLength], nLength);		// ת���ö����ݵ�Ŀ��PDU��

	// ����Ŀ���ַ�������
	return nDstLength;
}
// PDU���룬���ڽ��ա��Ķ�����Ϣ
// pSrc: ԴPDU��ָ��
// pDst: Ŀ��PDU����ָ��
// ����: �û���Ϣ������
int gprsDecodePdu(const char* pSrc, SM_PARAM* pDst)
{
	int nDstLength;			// Ŀ��PDU������
	unsigned char tmp;		// �ڲ��õ���ʱ�ֽڱ���
	unsigned char buf[256];	// �ڲ��õĻ�����

	// SMSC��ַ��Ϣ��
	gprsString2Bytes(pSrc, &tmp, 2);	// ȡ����
	tmp = (tmp - 1) * 2;	// SMSC���봮����
	pSrc += 4;			// ָ����ƣ�������SMSC��ַ��ʽ
	gprsSerializeNumbers(pSrc, pDst->SCA, tmp);	// ת��SMSC���뵽Ŀ��PDU��
	pSrc += tmp;		// ָ�����

	// TPDU�λ����������ظ���ַ��
	gprsString2Bytes(pSrc, &tmp, 2);	// ȡ��������
	pSrc += 2;		// ָ�����
//	if(tmp & 0x80)
	{
		// �����ظ���ַ��ȡ�ظ���ַ��Ϣ
		gprsString2Bytes(pSrc, &tmp, 2);	// ȡ����
		if(tmp & 1) tmp += 1;	// ������ż��
		pSrc += 4;			// ָ����ƣ������˻ظ���ַ(TP-RA)��ʽ
		gprsSerializeNumbers(pSrc, pDst->TPA, tmp);	// ȡTP-RA����
		pSrc += tmp;		// ָ�����
	}

	// TPDU��Э���ʶ�����뷽ʽ���û���Ϣ��
	gprsString2Bytes(pSrc, (unsigned char*)&pDst->TP_PID, 2);	// ȡЭ���ʶ(TP-PID)
	pSrc += 2;		// ָ�����
	gprsString2Bytes(pSrc, (unsigned char*)&pDst->TP_DCS, 2);	// ȡ���뷽ʽ(TP-DCS)
	pSrc += 2;		// ָ�����
	gprsSerializeNumbers(pSrc, pDst->TP_SCTS, 14);		// ����ʱ����ַ���(TP_SCTS) 
	pSrc += 14;		// ָ�����
	gprsString2Bytes(pSrc, &tmp, 2);	// �û���Ϣ����(TP-UDL)
	pSrc += 2;		// ָ�����
	if(pDst->TP_DCS == GSM_7BIT)	
	{
		// 7-bit����
		nDstLength = gprsString2Bytes(pSrc, buf, tmp & 7 ? (int)tmp * 7 / 4 + 2 : (int)tmp * 7 / 4);	// ��ʽת��
		gprsDecode7bit(buf, pDst->TP_UD, nDstLength);	// ת����TP-DU
		nDstLength = tmp;
	}
	else if(pDst->TP_DCS == GSM_UCS2)
	{
		// UCS2����
		nDstLength = gprsString2Bytes(pSrc, buf, tmp * 2);			// ��ʽת��
		nDstLength = gprsDecodeUcs2(buf, pDst->TP_UD, nDstLength);	// ת����TP-DU
	}
	else
	{
		// 8-bit����
		nDstLength = gprsString2Bytes(pSrc, buf, tmp * 2);			// ��ʽת��
		nDstLength = gprsDecode8bit(buf, pDst->TP_UD, nDstLength);	// ת����TP-DU
	}

	// ����Ŀ���ַ�������
	return nDstLength;
}
// ���Ͷ���Ϣ
// pSrc: ԴPDU����ָ��
BOOL gprsSendMessage(const SM_PARAM* pSrc)
{
	int nPduLength;		// PDU������
	unsigned char nSmscLength;	// SMSC������
	int nResult;		// 
	char cmd[16];		// ���
	char temp[16];
	char pdu[512];		// PDU��
	char ans[128];		// Ӧ��

	nPduLength = gprsEncodePdu(pSrc, pdu);	// ����PDU����������PDU��
	pdu[nPduLength] = 0;

	gprsString2Bytes(pdu, &nSmscLength, 2);	// ȡPDU���е�SMSC��Ϣ����
	nSmscLength++;		// ���ϳ����ֽڱ���

	// �����еĳ��ȣ�������SMSC��Ϣ���ȣ��������ֽڼ�
	sprintf(cmd, "AT+CMGS=%d\r", nPduLength / 2 - nSmscLength);	// ��������

	gprs_send_cmd(cmd);	// ��������

	Delay(500);
	
	nResult = gprs_recv_cmd(ans);	// ��Ӧ������
	nResult = gprs_recv_cmd(ans);

	// �����ܷ��ҵ�"\r\n> "�����ɹ����
	if(nResult == GPRS_OK)
	{
		int loopcnt = 40;
		nResult = gprs_recv_cmd(ans);
		gprs_send_cmd(pdu);  // �õ��϶��ش𣬼������PDU��

		pdu[0] = 0x1a; // CTRL + Z = 0x1a
		pdu[1] = 0x0;
		gprs_send_cmd(pdu);
		
		while(1)
		{
			Delay(10);
			gprs_recv_cmd(ans);	// ��Ӧ������

			// �����ܷ��ҵ�"+CMS ERROR"�����ɹ����
			if(strncmp(ans, "+CMS ERROR", 10) == 0)
			{
				TRACE("����ʧ��\n");
				return FALSE;
			}else if (strncmp(ans, "OK", 2) == 0)
			{
				TRACE("���ͳɹ�\n");
				return TRUE;
			}
			
			if(loopcnt-- == 0)
				return FALSE;
		}
	}
	TRACE("����ʧ��\n");
	gprs_recv_cmd(ans);
	gprs_recv_cmd(ans);
	return FALSE;
}
