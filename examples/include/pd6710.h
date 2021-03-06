#ifndef __PD6710_H__
#define __PD6710_H__

void Test_PD6710(void);


#define PD6710_MEM_BASE_ADDRESS     (0x10000000)	//nGCS2
#define PD6710_IO_BASE_ADDRESS      (0x11000000)	

#define rPD6710_INDEX	(*(volatile unsigned char *)(PD6710_IO_BASE_ADDRESS+0x3e0))
#define rPD6710_DATA	(*(volatile unsigned char *)(PD6710_IO_BASE_ADDRESS+0x3e1))


#define CHIP_REVISION			(0x0)
#define INTERFACE_STATUS		(0x1)
#define POWER_CTRL			(0x2)
#define INT_GENERAL_CTRL 		(0x3)
#define CARD_STAT_CHANGE		(0x4)
#define MANAGEMENT_INT_CONFIG 		(0x5)
#define MAPPING_ENABLE			(0x6)
#define IO_WINDOW_CTRL			(0x7)
#define SYS_IO_MAP0_START_L		(0x8)
#define SYS_IO_MAP0_START_H		(0x9)
#define SYS_IO_MAP0_END_L 		(0xa)
#define SYS_IO_MAP0_END_H 		(0xb)
#define SYS_IO_MAP1_START_L 		(0xc)
#define SYS_IO_MAP1_START_H 		(0xd)
#define SYS_IO_MAP1_END_L 		(0xe)
#define SYS_IO_MAP1_END_H 		(0xf)
#define SYS_MEM_MAP0_START_L 		(0x10)
#define SYS_MEM_MAP0_START_H 		(0x11)
#define SYS_MEM_MAP0_END_L 		(0x12)
#define SYS_MEM_MAP0_END_H 		(0x13)
#define CARD_MEM_MAP0_OFFSET_L 		(0x14)
#define CARD_MEM_MAP0_OFFSET_H 		(0x15)
#define MISC_CTRL1			(0x16)
#define FIFO_CTRL			(0x17)
#define SYS_MEM_MAP1_START_L 		(0x18)
#define SYS_MEM_MAP1_START_H 		(0x19)
#define SYS_MEM_MAP1_END_L 		(0x1a)
#define SYS_MEM_MAP1_END_H 		(0x1b)
#define CARD_MEM_MAP1_OFFSET_L 		(0x1c)
#define CARD_MEM_MAP1_OFFSET_H 		(0x1d)
#define MISC_CTRL2			(0x1e)
#define CHIP_INFO			(0x1f)
#define SYS_MEM_MAP2_START_L 		(0x20)
#define SYS_MEM_MAP2_START_H 		(0x21)
#define SYS_MEM_MAP2_END_L 		(0x22)
#define SYS_MEM_MAP2_END_H 		(0x23)
#define CARD_MEM_MAP2_OFFSET_L 		(0x24)
#define CARD_MEM_MAP2_OFFSET_H 		(0x25)
#define ATA_CTRL			(0x26)
#define SCRATCHPAD			(0x27)
#define SYS_MEM_MAP3_START_L 		(0x28)
#define SYS_MEM_MAP3_START_H 		(0x29)
#define SYS_MEM_MAP3_END_L 		(0x2a)
#define SYS_MEM_MAP3_END_H 		(0x2b)
#define CARD_MEM_MAP3_OFFSET_L 		(0x2c)
#define CARD_MEM_MAP3_OFFSET_H 		(0x2d)
#define EXTENDED_INDEX			(0x2e)
#define EXTENDED_DATA			(0x2f)
#define SYS_MEM_MAP4_START_L 		(0x30)
#define SYS_MEM_MAP4_START_H 		(0x31)
#define SYS_MEM_MAP4_END_L 		(0x32)
#define SYS_MEM_MAP4_END_H 		(0x33)
#define CARD_MEM_MAP4_OFFSET_L 		(0x34)
#define CARD_MEM_MAP4_OFFSET_H 		(0x35)
#define CARD_IO_MAP0_OFFSET_L 		(0x36)
#define CARD_IO_MAP0_OFFSET_H 		(0x37)
#define CARD_IO_MAP1_OFFSET_L 		(0x38)
#define CARD_IO_MAP1_OFFSET_H 		(0x39)
#define SETUP_TIMING0			(0x3a)
#define CMD_TIMING0			(0x3b)
#define RECOVERY_TIMING0		(0x3c)
#define SETUP_TIMING1			(0x3d)
#define CMD_TIMING1			(0x3e)
#define RECOVERY_TIMING1		(0x3f)


#endif /*__PD6710_H__*/
