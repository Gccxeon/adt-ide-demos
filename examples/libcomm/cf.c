#include "2410addr.h"
#include "2410lib.h"
#include "def.h"

#define CF_CONTROL_ADDR       0x28080000

#define CF_CONTROL_MASK_POWER  (1<<2)
#define CF_CONTROL_MASK_RESET  (1<<3)
#define CF_CONTROL_MASK_IOIS   (1<<4)
 
#define CF_MEMORY_ATTR_BASE   0x12000000
#define CF_MEMORY_COMMON_BASE 0x16000000
#define CF_IO_BASE            0x11000000

int cf_ctrl_value = 0x2;

/********************************************************************
// Function name	: cf_pwr_ctrl
// Description	    : CF卡电源控制
// Return type		: void
// Argument         : int bpwron ： 1打开电源，0关闭电源
*********************************************************************/
void cf_pwr_ctrl(int bpwron)
{
	if(bpwron)
	{
		cf_ctrl_value |= CF_CONTROL_MASK_POWER;
	}else
	{
		cf_ctrl_value &= ~CF_CONTROL_MASK_POWER;
	}
	*(unsigned char *)CF_CONTROL_ADDR = cf_ctrl_value;
}

/********************************************************************
// Function name	: cf_rst_ctrl
// Description	    : CF卡复位操作
// Return type		: void
// Argument         : int brst
*********************************************************************/
void cf_rst_ctrl(int brst)
{
	if(brst)
	{
		cf_ctrl_value |= CF_CONTROL_MASK_RESET;
	}else
	{
		cf_ctrl_value &= ~CF_CONTROL_MASK_RESET;
	}
	*(unsigned char *)CF_CONTROL_ADDR = cf_ctrl_value;
}

/********************************************************************
// Function name	: cf_iois_ctrl
// Description	    : CF卡IOIS信号控制
// Return type		: void
// Argument         : int bmemorywrite
*********************************************************************/
void cf_iois_ctrl(int bmemorywrite)
{
	if(bmemorywrite)
	{
		cf_ctrl_value |= CF_CONTROL_MASK_IOIS;
	}else
	{
		cf_ctrl_value &= ~CF_CONTROL_MASK_IOIS;
	}
	*(unsigned char *)CF_CONTROL_ADDR = cf_ctrl_value;
}

void cf_init()
{
	/* CF卡片选信号总线宽度设置 */
	rBWSCON &= (~(0xf<<8));
	rBWSCON |= (0x00<<8);

	rBANKCON2 = 0x6700;	
	Delay(10);
		
	/* 打开CF卡电源 */
	cf_pwr_ctrl(0);
	Delay(100);
		
	/* CF卡复位 */
	cf_rst_ctrl(0);
	Delay(3);
	cf_rst_ctrl(1);
	Delay(3);
	cf_rst_ctrl(0);
		
	Delay(100);
}

/********************************************************************
// Function name	: cf_read_cis
// Description	    : 读取CF卡卡信息
// Return type		: void
// Argument         : unsigned char *cisstring
*********************************************************************/
void cf_read_cis(unsigned char *cisstring)
{
	unsigned char CISdata[180];
	unsigned char data;
	int i;
	for (i=0;i<180;i++)
	{ 
		if(i % 0x8 == 0)
			Uart_Printf("\n0x%08x    : ", i*2);
		//copy CIS to CISdata[]
		data =*(unsigned char *) (CF_MEMORY_ATTR_BASE+i*2);// CIS can be access in even address only
		CISdata[i] = (data & 0xff);
		
//		Uart_Printf("0x%02x(%c) ", CISdata[i], CISdata[i]);
		Uart_Printf("0x%02x ", CISdata[i]);
	}
	cisstring[0] = 0;
	for (i=25; i<45 ; i++) 
	{
		if (CISdata[i]!='\0')
			sprintf(cisstring, "%s%c", cisstring, CISdata[i]);
		else 
			sprintf(cisstring, "%s%c", cisstring, '\n');
	}
}

/********************************************************************
// Function name	: cf_memory_write_block
// Description	    : 写数据到CF卡，每个BLOCK 512字节
// Return type		: void
// Argument         : unsigned char *data：待写入数据
// Argument         :  int sectnum：BLOCK号
*********************************************************************/
void cf_memory_write_block(unsigned char *data, int sectnum)
{
	unsigned char tempU8;
	unsigned short i;
	//***Set sector count***
	*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000002) = 0x1;//sector count =1
	//***Set the LBA address of memory block to be written***
	*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000003) = sectnum + 32; //LBA [7:0] =1
	*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000004) = 0x0; //LBA [15:8:] =0
	*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000005) = 0x0; //LBA [23:16:] =0
	*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000006) = 0xE0; //LBA [27:24]=0 (lower 4 bit of register)
	//Issue write command
	*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000007) = 0x30;//issue 30H command for sector write
	//Poll for busy bit
	tempU8 = *(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000007);
	while ( tempU8&0x80) 
	{
		//poll for busy bit (bit 7 of register), quit loop when busy bit =0
		tempU8 = *(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000007);
	}
	//Write data to data buffer until DRQ is clear
	for (i=0;(tempU8&0x08)==0x08;i++) 
	{
		*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000008)=data[i];// write 2 byte of data to data buffer
		tempU8=*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000007);// poll for DRQ (bit 3 of register)
	}
//	Uart_Printf("Finish Writing\n");	
}

/********************************************************************
// Function name	: cf_memory_read_block
// Description	    : 从CF卡中读取数据
// Return type		: void
// Argument         : unsigned char *data：保存数据的缓冲区
// Argument         :  int sectnum：BLOCK号
*********************************************************************/
void cf_memory_read_block(unsigned char *data, int sectnum)
{
	unsigned char tempU8;
	int i;
	//Set sector count
	*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000002) = 0x1;//sector count =1
	//Set the LBA address of memory block to be read
	*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000003) = sectnum + 32; //LBA [7:0] =1
	*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000004) = 0x0; //LBA [15:8] =0
	*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000005) = 0x0; //LBA [23:16] =0
	*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000006) = 0xE0; //LBA [27:24] =0 (lower 4 bit of register)
	//Issue read command
	*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000007) = 0x20;//issue 20H command for sector read
	//Poll for busy bit
	tempU8 = *(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000007);
	while ( tempU8&0x80) 
	{
		//poll for busy bit (bit 7 of register), quit loop when busy bit =0
		tempU8 = *(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000007);
	}
	//Read data from buffer into U16data[] until DRQ is clear
	for (i=0;(tempU8&0x08)==0x08;i++) 
	{
		data[i]=*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000008);// read 2 byte of data from buffer
		tempU8=*(unsigned char *) (CF_MEMORY_COMMON_BASE+0x000007);// poll for DRQ (bit 3 of register)
	}
	Uart_Printf("Finish Reading\n");	
}
