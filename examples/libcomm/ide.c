#include "2410addr.h"
#include "2410lib.h"
#include "def.h"

#define IDE_CONTROL_ADDR       0x28000004

#define IDE_CONTROL_MASK_POWER  (1<<5)
#define IDE_CONTROL_MASK_IOIS   (1<<4)
#define IDE_CONTROL_MASK_CS     (0x3<<2)

#define inp(port) (*(unsigned char *)(port))
#define inw(port) (*(unsigned short *)(port))
#define outp(port, value) (*(unsigned char *)(port))=value

char *getascii (unsigned short in_data [], unsigned short off_start, unsigned short off_end);

int ide_ctrl_value = (1<<5);

void ide_iois_ctrl(int biois)
{
	if(biois)
	{
		ide_ctrl_value |= IDE_CONTROL_MASK_IOIS;
	}else
	{
		ide_ctrl_value &= ~IDE_CONTROL_MASK_IOIS;
	}
	*(unsigned char *)IDE_CONTROL_ADDR = ide_ctrl_value;
}

void ide_rst_ctrl(int brst)
{
	if(brst)
	{
		ide_ctrl_value |= IDE_CONTROL_MASK_POWER;
	}else
	{
		ide_ctrl_value &= ~IDE_CONTROL_MASK_POWER;
	}
	*(unsigned char *)IDE_CONTROL_ADDR = ide_ctrl_value;
}

void ide_cs_ctrl(int cs)
{
	ide_ctrl_value &= ~IDE_CONTROL_MASK_CS;
	ide_ctrl_value |= (cs<<2);
	*(unsigned char *)IDE_CONTROL_ADDR = ide_ctrl_value;
}

char *getascii (unsigned short in_data [], unsigned short off_start, unsigned short off_end)
{
	static char ret_val [255];
	int loop, loop1;
	
	for (loop = off_start, loop1 = 0; loop <= off_end; loop++)
    {
		ret_val [loop1++] = (char) (in_data [loop] / 256);  /* Get High byte */
		ret_val [loop1++] = (char) (in_data [loop] % 256);  /* Get Low byte */
    }
	ret_val [loop1] = '\0';  /* Make sure it ends in a NULL character */
	return (ret_val);
}

//Control Block Registers 
#define REG_ALT_STATUS			(IDE_CONTROL_BASE+0x04)		// R
#define REG_DEVICE_CONTROL		(IDE_CONTROL_BASE+0x04)		// W
#define REG_DRIVE_ADDRESS		(IDE_CONTROL_BASE+0x07)		// R

//Command Block Registers
#define REG_DATA				(IDE_COMMAND_BASE+0x00)		// RW
#define REG_ERROR				(IDE_COMMAND_BASE+0x01)		// RW
#define REG_SECTOR_COUNT		(IDE_COMMAND_BASE+0x02)		// RW
#define REG_SECTOR_NUMBER		(IDE_COMMAND_BASE+0x03)		// RW
#define REG_LBA_BITS_00_07		(IDE_COMMAND_BASE+0x03)		// RW
#define REG_CYLINDER_LOW		(IDE_COMMAND_BASE+0x04)		// RW
#define REG_LBA_BITS_08_15		(IDE_COMMAND_BASE+0x04)		// RW
#define REG_CYLINDER_HIGH		(IDE_COMMAND_BASE+0x05)		// RW
#define REG_LBA_BITS_16_23		(IDE_COMMAND_BASE+0x05)		// RW
#define REG_DRIVE_HEAD			(IDE_COMMAND_BASE+0x06)		// RW
#define REG_LBA_BITS_24_27		(IDE_COMMAND_BASE+0x06)		// RW
#define REG_STATUS				(IDE_COMMAND_BASE+0x07)		// R
#define REG_COMMAND				(IDE_COMMAND_BASE+0x07)		// W

// Status consts
#define IDE_ST_BUSY				0x80
#define IDE_ST_DRDY				0x40
#define IDE_ST_DWF				0x20
#define IDE_ST_DSC				0x10
#define IDE_ST_DRQ				0x08
#define IDE_ST_CORR				0x04
#define IDE_ST_IDX				0x02
#define IDE_ST_ERR				0x01

// Commands
#define IDE_CMD_READ_DMA		0xc8
#define IDE_CMD_READ_DMA1		0xc9
#define IDE_CMD_READ_LONG		0x22
#define IDE_CMD_READ_LONG1		0x23
#define IDE_CMD_READ_MULTI		0xc4
#define IDE_CMD_READ_SECTORS	0x20
#define IDE_CMD_READ_SECTORS1	0x21
#define IDE_CMD_READ_VERIFY_SECTORS		0x40
#define IDE_CMD_READ_VERIFY_SECTORS1	0x41

void ide_read_sector(
		unsigned char 	sec_cnt, 
		unsigned char 	sec_num, 
		unsigned short 	clyn, 
		unsigned char 	head, 
		unsigned char* 	p_data)
{
	int 			index;
	unsigned char 	status;
	unsigned char*	p_buffer = p_data;
	
	// switch to command block registers and wait for no busy
	ide_cs_ctrl(2);
	Delay(100);
	
//	rBWSCON &= (~(0xf<<8));
//	rBWSCON |= (0x01<<8);
//
//	rBANKCON2 = 0x7ffc;	
		
	Delay(600);
	
	/* Wait for controller not busy */
	do
	{
		status = inp (IDE_COMMAND_BASE + 14);
	}while (status & 0x80);
	
	/* step 1
	a) 	The host writes any required parameters to the Features, Sector Count, 
    	Sector Number, Cylinder and Drive/Head registers.*/
    outp(REG_SECTOR_COUNT, 	sec_cnt);
    outp(REG_SECTOR_NUMBER, sec_num);
    
    outp(REG_CYLINDER_LOW, 	clyn&0x00ff);
    outp(REG_CYLINDER_HIGH, (clyn>>8)&0x00ff);
    
    outp(REG_DRIVE_HEAD, 	0xa|(head&0x0f));		// CHS mode and master device
  	
    /* step 2
 	b) 	The host writes the command code to the Command Register. */
 	outp(REG_COMMAND, 		IDE_CMD_READ_SECTORS);
 	
 	/* step 3
 	c) 	The drive sets BSY and prepares for data transfer. */
	/* Wait for controller busy */
//	do
//	{
		status = inp (IDE_COMMAND_BASE + 14);
//	}while (!(status & 0x80));
 	
 	/* step 4
 	d) 	When a sector of data is available, the drive sets DRQ and clears BSY 
    	prior to asserting INTRQ. */
AGAIN:    	
    do
	{
		status = inp (IDE_COMMAND_BASE + 14);
	}while ((status & 0x80));
	
    /* step 5
 	e) 	After detecting INTRQ, the host reads the Status Register, then reads one 
    	sector of data via the Data Register. In response to the Status Register 
    	being read, the drive negates INTRQ.  */
    for (index = 0; index != 512; index += 2) 	/* Read "sector" */
	{
		*p_buffer = inw (REG_DATA);
	}
	
	sec_cnt--;
    	
    /* step 6
 	f) 	The drive clears DRQ. If transfer of another sector is required, the drive 
    	also sets BSY and the above sequence is repeated from d).*/
    if(sec_cnt!=0) goto AGAIN;
}

void dump_data(unsigned char* data, int size)
{
	int loopcnt;
	for (loopcnt = 0; loopcnt < size; loopcnt ++)
	{
		Uart_Printf("0x%02x ", data[loopcnt]);
		if(loopcnt != 0 && (loopcnt % 16 == 0))		
			Uart_Printf("\n");
	}
}

