#include "2410addr.h"
#include "2410lib.h"
#include "def.h"
#include "2410IIC.h"

////////////////////////////////////////////////////////////////////////////////////////
#define TX_DONE 0
#define TX_SENT 1
#define TX_SEND 2

static unsigned char ncodes;
static unsigned char bi;
static volatile int tx_state;
static unsigned char status;
static unsigned char buffer[4];

#define ESCE0(x)	(0xe000|(x))
#define ESCE1(x)	(0xe100|(x))

#define KBD_ESCAPEE0	0xe0		/* in */
#define KBD_ESCAPEE1	0xe1		/* in */

#define KBD_BAT		0xaa		/* in */
#define KBD_SETLEDS	0xed		/* out */
#define KBD_ECHO	0xee		/* in/out */
#define KBD_BREAK	0xf0		/* in */
#define KBD_TYPRATEDLY	0xf3		/* out */
#define KBD_SCANENABLE	0xf4		/* out */
#define KBD_DEFDISABLE	0xf5		/* out */
#define KBD_DEFAULT	0xf6		/* out */
#define KBD_ACK		0xfa		/* in */
#define KBD_DIAGFAIL	0xfd		/* in */
#define KBD_RESEND	0xfe		/* in/out */
#define KBD_RESET	0xff		/* out */

#define CODE_BREAK	1
#define CODE_ESCAPEE0	2
#define CODE_ESCAPEE1	4
#define CODE_ESCAPE12	8

#define K_NONE		0x7f
#define K_ESC		0x00
#define K_F1		0x01
#define K_F2		0x02
#define K_F3		0x03
#define K_F4		0x04
#define K_F5		0x05
#define K_F6		0x06
#define K_F7		0x07
#define K_F8		0x08
#define K_F9		0x09
#define K_F10		0x0a
#define K_F11		0x0b
#define K_F12		0x0c
#define K_PRNT		0x0d
#define K_SCRL		0x0e
#define K_BRK		0x0f
#define K_AGR		0x10
#define K_1		0x11
#define K_2		0x12
#define K_3		0x13
#define K_4		0x14
#define K_5		0x15
#define K_6		0x16
#define K_7		0x17
#define K_8		0x18
#define K_9		0x19
#define K_0		0x1a
#define K_MINS		0x1b
#define K_EQLS		0x1c
#define K_BKSP		0x1e
#define K_INS		0x1f
#define K_HOME		0x20
#define K_PGUP		0x21
#define K_NUML		0x22
#define KP_SLH		0x23
#define KP_STR		0x24
#define KP_MNS		0x3a
#define K_TAB		0x26
#define K_Q		0x27
#define K_W		0x28
#define K_E		0x29
#define K_R		0x2a
#define K_T		0x2b
#define K_Y		0x2c
#define K_U		0x2d
#define K_I		0x2e
#define K_O		0x2f
#define K_P		0x30
#define K_LSBK		0x31
#define K_RSBK		0x32
#define K_ENTR		0x47
#define K_DEL		0x34
#define K_END		0x35
#define K_PGDN		0x36
#define KP_7		0x37
#define KP_8		0x38
#define KP_9		0x39
#define KP_PLS		0x4b
#define K_CAPS		0x5d
#define K_A		0x3c
#define K_S		0x3d
#define K_D		0x3e
#define K_F		0x3f
#define K_G		0x40
#define K_H		0x41
#define K_J		0x42
#define K_K		0x43
#define K_L		0x44
#define K_SEMI		0x45
#define K_SQOT		0x46
#define K_HASH		0x1d
#define KP_4		0x48
#define KP_5		0x49
#define KP_6		0x4a
#define K_LSFT		0x4c
#define K_BSLH		0x33
#define K_Z		0x4e
#define K_X		0x4f
#define K_C		0x50
#define K_V		0x51
#define K_B		0x52
#define K_N		0x53
#define K_M		0x54
#define K_COMA		0x55
#define K_DOT		0x56
#define K_FSLH		0x57
#define K_RSFT		0x58
#define K_UP		0x59
#define KP_1		0x5a
#define KP_2		0x5b
#define KP_3		0x5c
#define KP_ENT		0x67
#define K_LCTL		0x3b
#define K_LALT		0x5e
#define K_SPCE		0x5f
#define K_RALT		0x60
#define K_RCTL		0x61
#define K_LEFT		0x62
#define K_DOWN		0x63
#define K_RGHT		0x64
#define KP_0		0x65
#define KP_DOT		0x66

// º¸≈Ã…®√Ë¬Î
static unsigned char keycode_translate[256] =
{
/* 00 */  K_NONE, K_F9  , K_NONE, K_F5  , K_F3  , K_F1  , K_F2  , K_F12 ,
/* 08 */  K_NONE, K_F10 , K_F8  , K_F6  , K_F4  , K_TAB , K_AGR , K_NONE,
/* 10 */  K_NONE, K_LALT, K_LSFT, K_NONE, K_LCTL, K_Q   , K_1   , K_NONE,
/* 18 */  K_NONE, K_NONE, K_Z   , K_S   , K_A   , K_W   , K_2   , K_NONE,
/* 20 */  K_NONE, K_C   , K_X   , K_D   , K_E   , K_4   , K_3   , K_NONE,
/* 28 */  K_NONE, K_SPCE, K_V   , K_F   , K_T   , K_R   , K_5   , K_NONE,
/* 30 */  K_NONE, K_N   , K_B   , K_H   , K_G   , K_Y   , K_6   , K_NONE,
/* 38 */  K_NONE, K_NONE, K_M   , K_J   , K_U   , K_7   , K_8   , K_NONE,
/* 40 */  K_NONE, K_COMA, K_K   , K_I   , K_O   , K_0   , K_9   , K_NONE,
/* 48 */  K_NONE, K_DOT , K_FSLH, K_L   , K_SEMI, K_P   , K_MINS, K_NONE,
/* 50 */  K_NONE, K_NONE, K_SQOT, K_NONE, K_LSBK, K_EQLS, K_NONE, K_NONE,
/* 58 */  K_CAPS, K_RSFT, K_ENTR, K_RSBK, K_NONE, K_HASH, K_NONE, K_NONE,
/* 60 */  K_NONE, K_BSLH, K_NONE, K_NONE, K_NONE, K_NONE, K_BKSP, K_NONE,
/* 68 */  K_NONE, KP_1  , K_NONE, KP_4  , KP_7  , K_NONE, K_NONE, K_NONE,
/* 70 */  KP_0  , KP_DOT, KP_2  , KP_5  , KP_6  , KP_8  , K_ESC , K_NUML,
/* 78 */  K_F11 , KP_PLS, KP_3  , KP_MNS, KP_STR, KP_9  , K_SCRL, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_F7  , K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE,
	  K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE, K_NONE
};
static unsigned char ps2kbd_sysrq_xlate[] = 
{
    27,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,
   '`',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
   '8',  '9',  '0',  '-',  '=',  '£',  127,    0,
     0,    0,    0,  '/',  '*',  '#',    9,  'q',
   'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',
   'p',  '[',  ']', '\\',  22,    23,   25,  '7',
   '8',  '9',  '-',    0,  'a',  's',  'd',  'f',
   'g',  'h',  'j',  'k',  'l',  ';', '\'',   13,
   '4',  '5',  '6',  '+',    0,    0,  'z',  'x',
   'c',  'v',  'b',  'n',  'm',  ',',  '.',  '/',
     0,    0,  '1',  '2',  '3',    0,    0,  ' ',
     0,    0,    0,    0,    0,  '0',  '.',   10,
     0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,
};
////////////////////////////////////////////////////////////////////////////////////////

/*
 * PS/2 Û±Í:
 *     TCLK : GPF7
 *     DATA : GPB4
 * PS/2º¸≈Ã:
 *     TCLK : GPG0
 *     DATA : GPG11
 */

/* º¸≈Ã */
#define PS2_KBD_SET_CLK_OUT      rGPFCON = rGPFCON & (~ (0x3 << 14)) | (0x01 << 14) /* …Ë÷√ ±÷”œﬂŒ™ ‰≥ˆ */
#define PS2_KBD_SET_CLK_INPUT    rGPFCON &= ~ (0x3 << 14)/* …Ë÷√ ±÷”œﬂŒ™ ‰»Î */
#define PS2_KBD_SET_DATA_OUT     rGPBCON = rGPBCON & (~ (0x3 << 8)) | (0x01 << 8)/* …Ë÷√ ˝æ›œﬂŒ™ ‰≥ˆ */
#define PS2_KBD_SET_DATA_INPUT   rGPBCON &= ~ (0x3 << 8)/* …Ë÷√ ˝æ›œﬂŒ™ ‰»Î */

#define PS2_KBD_CLK_READ()      ((rGPFDAT) & (1 << 7)) >> 7  /* ªÒ»° ±÷”–≈∫≈ */
#define PS2_KBD_CLK_WRITE(CLK)     rGPFDAT = rGPFDAT & (~(1 << 7)) | ((CLK) << 7)   /*  ‰≥ˆ ±÷” */
#define PS2_KBD_DATA_READ()     ((rGPBDAT) & (1 << 4)) >> 4    /* ¥” ˝æ›œﬂ∂¡ ˝æ› */
#define PS2_KBD_DATA_WRITE(DATA)   rGPBDAT = rGPBDAT & (~(1 << 4)) | ((DATA) << 4)    /* Õ˘ ˝æ›œﬂ–¥ ˝æ› */
//#define PS2_KBD_SET_CLK_OUT      rGPBCON = rGPBCON & (~ (0x3 << 8)) | (0x01 << 8) /* …Ë÷√ ±÷”œﬂŒ™ ‰≥ˆ */
//#define PS2_KBD_SET_CLK_INPUT    rGPBCON &= ~ (0x3 << 8)/* …Ë÷√ ±÷”œﬂŒ™ ‰»Î */
//#define PS2_KBD_SET_DATA_OUT     rGPBCON = rGPBCON & (~ (0x3 << 4)) | (0x01 << 4)/* …Ë÷√ ˝æ›œﬂŒ™ ‰≥ˆ */
//#define PS2_KBD_SET_DATA_INPUT   rGPBCON &= ~ (0x3 << 4)/* …Ë÷√ ˝æ›œﬂŒ™ ‰»Î */
//
//#define PS2_KBD_CLK_READ()      ((rGPBDAT) & (1 << 4)) >> 4  /* ªÒ»° ±÷”–≈∫≈ */
//#define PS2_KBD_CLK_WRITE(CLK)     rGPBDAT = rGPBDAT & (~(1 << 4)) | ((CLK) << 4)   /*  ‰≥ˆ ±÷” */
//#define PS2_KBD_DATA_READ()     ((rGPBDAT) & (1 << 2)) >> 2    /* ¥” ˝æ›œﬂ∂¡ ˝æ› */
//#define PS2_KBD_DATA_WRITE(DATA)   rGPBDAT = rGPGDAT & (~(1 << 2)) | ((DATA) << 2)    /* Õ˘ ˝æ›œﬂ–¥ ˝æ› */

/*  Û±Í */
#define PS2_MOUSE_SET_CLK_OUT      rGPGCON = rGPGCON & (~ (0x3 << 0)) | (0x01 << 0) /* …Ë÷√ ±÷”œﬂŒ™ ‰≥ˆ */
#define PS2_MOUSE_SET_CLK_INPUT    rGPGCON &= ~ (0x3 << 0)/* …Ë÷√ ±÷”œﬂŒ™ ‰»Î */
#define PS2_MOUSE_SET_DATA_OUT     rGPGCON = rGPGCON & (~ (0x3 << 22)) | (0x01 << 22)/* …Ë÷√ ˝æ›œﬂŒ™ ‰≥ˆ */
#define PS2_MOUSE_SET_DATA_INPUT   rGPGCON &= ~ (0x3 << 22)/* …Ë÷√ ˝æ›œﬂŒ™ ‰»Î */

#define PS2_MOUSE_CLK_READ()      ((rGPGDAT) & (1 << 0)) >> 0  /* ªÒ»° ±÷”–≈∫≈ */
#define PS2_MOUSE_CLK_WRITE(CLK)     rGPGDAT = rGPGDAT & (~(1 << 0)) | ((CLK) << 0)   /*  ‰≥ˆ ±÷” */
#define PS2_MOUSE_DATA_READ()     ((rGPGDAT) & (1 << 11)) >> 11    /* ¥” ˝æ›œﬂ∂¡ ˝æ› */
#define PS2_MOUSE_DATA_WRITE(DATA)   rGPGDAT = rGPGDAT & (~(1 << 11)) | ((DATA) << 11)    /* Õ˘ ˝æ›œﬂ–¥ ˝æ› */
//#define PS2_MOUSE_SET_CLK_OUT      rGPGCON = rGPGCON & (~ (0x3 << 22)) | (0x01 << 22) /* …Ë÷√ ±÷”œﬂŒ™ ‰≥ˆ */
//#define PS2_MOUSE_SET_CLK_INPUT    rGPGCON &= ~ (0x3 << 22)/* …Ë÷√ ±÷”œﬂŒ™ ‰»Î */
//#define PS2_MOUSE_SET_DATA_OUT     rGPBCON = rGPBCON & (~ (0x3 << 6)) | (0x01 << 6)/* …Ë÷√ ˝æ›œﬂŒ™ ‰≥ˆ */
//#define PS2_MOUSE_SET_DATA_INPUT   rGPBCON &= ~ (0x3 << 6)/* …Ë÷√ ˝æ›œﬂŒ™ ‰»Î */
//
//#define PS2_MOUSE_CLK_READ()      ((rGPGDAT) & (1 << 11)) >> 11  /* ªÒ»° ±÷”–≈∫≈ */
//#define PS2_MOUSE_CLK_WRITE(CLK)     rGPGDAT = rGPGDAT & (~(1 << 11)) | ((CLK) << 11)   /*  ‰≥ˆ ±÷” */
//#define PS2_MOUSE_DATA_READ()     ((rGPBDAT) & (1 << 3)) >> 3    /* ¥” ˝æ›œﬂ∂¡ ˝æ› */
//#define PS2_MOUSE_DATA_WRITE(DATA)   rGPBDAT = rGPBDAT & (~(1 << 3)) | ((DATA) << 3)    /* Õ˘ ˝æ›œﬂ–¥ ˝æ› */

#define PS2_HIGH           1 /* ∏ﬂµÁ∆Ω */
#define PS2_LOW            0 /* µÕµÁ∆Ω */

#define CMD_ENABLE_DATA_REPORT   0xF4
#define CMD_SET_SAMP_RATE        0xF3
#define CMD_GET_DEV_ID           0xF2
#define CMD_RESET                0xFF
/********************************************************************
// Function name	: PS2_Mouse_Init
// Description	    : PS/2 Û±Í…Ë±∏≥ı ºªØ
// Return type		: void
// Argument         : 
*********************************************************************/
void PS2_Mouse_Init()
{
	char ch;


	// ∏¥Œª	
	PS2_Mouse_Put_Byte(CMD_RESET);
	ch = PS2_Mouse_Get_Byte(); 
	ch = PS2_Mouse_Get_Byte(); 
	
	PS2_Mouse_Put_Byte(CMD_RESET);
	ch = PS2_Mouse_Get_Byte(); 
	ch = PS2_Mouse_Get_Byte(); 

	PS2_Mouse_Put_Byte(CMD_RESET);
	ch = PS2_Mouse_Get_Byte(); 
	ch = PS2_Mouse_Get_Byte(); 

	// Ω¯»ÎSTREAMƒ£ Ω
	PS2_Mouse_Put_Byte(0xEA);
	ch = PS2_Mouse_Get_Byte(); 
	
	Delay(100);
	
#if 0	
	// Ω¯»ÎŒ¢»ÌIntellimouseƒ£ Ω
	PS2_Mouse_Put_Byte(CMD_SET_SAMP_RATE);
	ch = PS2_Mouse_Get_Byte();
	if(ch != 0xfa)
	{
		Uart_Printf("PS2_Mouse_Init Error(0x%x)\n", ch);
		return ;
	}
	PS2_Mouse_Put_Byte(200);

	PS2_Mouse_Put_Byte(CMD_SET_SAMP_RATE);
	ch = PS2_Mouse_Get_Byte();
	if(ch != 0xfa)
	{
		Uart_Printf("PS2_Mouse_Init Error(0x%x)\n", ch);
		return ;
	}
	PS2_Mouse_Put_Byte(100);
	
	PS2_Mouse_Put_Byte(CMD_SET_SAMP_RATE);
	ch = PS2_Mouse_Get_Byte();
	if(ch != 0xfa)
	{
		Uart_Printf("PS2_Mouse_Init Error(0x%x)\n", ch);
		return ;
	}
	PS2_Mouse_Put_Byte(80);
#endif
	
	// ªÒ»°…Ë±∏ID
	PS2_Mouse_Put_Byte(CMD_GET_DEV_ID);
	ch = PS2_Mouse_Get_Byte(); Uart_Printf("From Mouse 0x%x\n", ch);

	//  Û±Í∑¢ÀÕ◊‘º∫µƒ…Ë±∏ID∏¯÷˜ª˙∫ÛÀ¸æÕΩ¯»Î¡ÀStream ƒ£ Ω◊¢“‚ Û±Í…Ë÷√µƒ“ª∏ˆ»± °÷µ÷Æ“ª « ˝æ›
	// ±®∏Ê±ªΩ˚÷π’‚æÕ“‚Œ∂◊≈ Û±Í‘⁄√ª ’µΩ πƒ‹ ˝æ›±®∏Ê0xF4 √¸¡Ó÷Æ«∞≤ªª·∑¢ÀÕ»Œ∫ŒŒª“∆ ˝æ›∞¸
	// ∏¯÷˜ª˙
	PS2_Mouse_Put_Byte(CMD_ENABLE_DATA_REPORT);
	ch = PS2_Mouse_Get_Byte();
	if(ch != 0xfa)
	{
		Uart_Printf("PS2_Mouse_Init Error(0x%x)\n", ch);
		return ;
	}
	// ªÒ»°…Ë±∏ID
	PS2_Mouse_Put_Byte(CMD_GET_DEV_ID);
	ch = PS2_Mouse_Get_Byte(); Uart_Printf("From Mouse 0x%x\n", ch);
}

/********************************************************************
// Function name	: PS2_Mouse_Put_Byte
// Description	    : ∑¢ÀÕ“ª∏ˆ◊÷Ω⁄µΩ Û±Í
// Return type		: void
// Argument         : char byte
*********************************************************************/
void PS2_Mouse_Put_Byte(char byte)
{
	char clk, bit, data, par = 0;
	int loopcnt = 0;

	// 1. ∞— ±÷”œﬂ¿≠µÕ÷¡…Ÿ100 Œ¢√Î
	PS2_MOUSE_SET_CLK_OUT;
	PS2_MOUSE_CLK_WRITE(PS2_LOW);
	for(loopcnt = 0; loopcnt < 1000; loopcnt++);
	
	// 2. ∞— ˝æ›œﬂ¿≠µÕ
	PS2_MOUSE_SET_DATA_OUT;
	PS2_MOUSE_DATA_WRITE(PS2_LOW);
	
	// 3.  Õ∑≈ ±÷”œﬂ
	PS2_MOUSE_SET_CLK_INPUT;
	
	// 4. µ»¥˝…Ë±∏∞— ±÷”œﬂ¿≠µÕ
	do
	{
		clk = PS2_MOUSE_CLK_READ();
	}while(clk);
	
	// 5. …Ë÷√/∏¥Œª ˝æ›œﬂ∑¢ÀÕµ⁄“ª∏ˆ ˝æ›Œª
	for(loopcnt = 0; loopcnt < 8; loopcnt++)
	{
		PS2_MOUSE_DATA_WRITE(byte & 0x01);
		if(byte & 0x01) par++;
		byte = (byte >> 1);

		// 6. µ»¥˝…Ë±∏∞— ±÷”œﬂ¿≠∏ﬂ
		do
		{
			clk = PS2_MOUSE_CLK_READ();
		}while(!clk);
		
		// 7. µ»¥˝…Ë±∏∞— ±÷”œﬂ¿≠µÕ
		do
		{
			clk = PS2_MOUSE_CLK_READ();
		}while(clk);
	}	
		
	// 5. ∑¢ÀÕ–£—ÈŒª
	PS2_MOUSE_DATA_WRITE((~(par % 2)) & 0x01);

	// 6. µ»¥˝…Ë±∏∞— ±÷”œﬂ¿≠∏ﬂ
	do
	{
		clk = PS2_MOUSE_CLK_READ();
	}while(!clk);
		
	// 7. µ»¥˝…Ë±∏∞— ±÷”œﬂ¿≠µÕ
	do
	{
		clk = PS2_MOUSE_CLK_READ();
	}while(clk);
	
	// 8.  Õ∑≈ ˝æ›œﬂ
	PS2_MOUSE_SET_DATA_INPUT;
	
	// 9. µ»¥˝…Ë±∏∞— ˝æ›œﬂ¿≠µÕ
	do
	{
		data = PS2_MOUSE_DATA_READ();
	}while(data);
	
	// 10. µ»¥˝…Ë±∏∞— ±÷”œﬂ¿≠µÕ
	do
	{
		clk = PS2_MOUSE_CLK_READ();
	}while(clk);
	
	// 11. µ»¥˝…Ë±∏ Õ∑≈ ˝æ›œﬂ∫Õ ±÷”œﬂ
	do
	{
		data = PS2_MOUSE_DATA_READ();
	}while(!data);
	do
	{
		clk = PS2_MOUSE_CLK_READ();
	}while(!clk);
}

/********************************************************************
// Function name	: PS2_Mouse_Get_Bit
// Description	    : ¥”PS/2 Û±Í…Ë±∏∂¡»°“ª∏ˆBit
// Return type		: char
// Argument         : void
*********************************************************************/
char PS2_Mouse_Get_Bit(void)
{
	char clk, bit;
	
	// µ»¥˝ ±÷”œﬂ±‰Œ™µÕµÁ∆Ω
	do
	{
		clk = PS2_MOUSE_CLK_READ();
	}while(clk);
	
	// ∂¡»° ˝æ›
	bit = PS2_MOUSE_DATA_READ();
	
	// µ»¥˝ ±÷”œﬂ±‰Œ™∏ﬂµÁ∆Ω
	do
	{
		clk = PS2_MOUSE_CLK_READ();
	}while(!clk);

	return bit;
}

/********************************************************************
// Function name	: PS2_Mouse_Get_Byte
// Description	    : ¥”PS/2 Û±Í∂¡»°“ª∏ˆ◊÷Ω⁄
// Return type		: char
// Argument         : void
*********************************************************************/
char PS2_Mouse_Get_Byte(void)
{
	int loopcnt;
	char bit, data = 0;
	
	bit = PS2_Mouse_Get_Bit(); // ø™ ºŒª
	//  ˝æ›Œª
	
	for(loopcnt = 0; loopcnt < 8; loopcnt++)
	{
		data = (PS2_Mouse_Get_Bit() << loopcnt) | data; 
	}
	
	bit = PS2_Mouse_Get_Bit(); // –£—ÈŒª
	bit = PS2_Mouse_Get_Bit(); // Ω· ¯Œª
	
	return data;
}

/********************************************************************
// Function name	: PS2_Mouse_Handle_Rawcode
// Description	    : ±Í◊ºPS/2 Û±Í‘≠ º ˝æ›¥¶¿Ì¥˙¬Î
// Return type		: void
// Argument         : char ch
*********************************************************************/
void PS2_Mouse_Handle_Rawcode(char ch)
{
#define MOUSE_CMD     0
#define MOUSE_DATA1   1
#define MOUSE_DATA2   2
#define MOUSE_DATA3   3

	static int mouse_status = MOUSE_CMD;
	
	if(mouse_status == MOUSE_CMD)
	{
		if(ch & 0x01)
		{
			Uart_Printf("Left Button Clicked\n");
		}
		if(ch & 0x02)
		{
			Uart_Printf("Right Button Clicked\n");
		}
		if(ch & 0x04)
		{
			Uart_Printf("Middle Button Clicked\n");
		}
	}else if(mouse_status == MOUSE_DATA1)
	{
		if((ch & 0x01) || (ch & 0x02) || (ch & 0x04))
		{
			// button click
		}
		mouse_status = MOUSE_DATA2;
	}else if(mouse_status == MOUSE_DATA2)
	{
		mouse_status = MOUSE_CMD;
	}
}

/********************************************************************
// Function name	: PS2_KBD_Init
// Description	    : PS/2º¸≈Ã…Ë±∏≥ı ºªØ
// Return type		: void
// Argument         : 
*********************************************************************/
void PS2_KBD_Init()
{
	// º¸≈Ã≥ı ºªØ: 
	//     …Ë÷√ ±÷”œﬂŒ™ ‰≥ˆ≤¢ ‰≥ˆµÕµÁ∆Ω
	//     …Ë÷√ ˝æ›œﬂŒ™ ‰≥ˆ≤¢ ‰≥ˆµÕµÁ∆Ω
	PS2_KBD_SET_CLK_OUT;
	PS2_KBD_CLK_WRITE(PS2_LOW);
	
	PS2_KBD_SET_DATA_OUT;
	PS2_KBD_DATA_WRITE(PS2_LOW);
	
	
	/////////////////////////////////
	Delay(100);
	PS2_KBD_SET_DATA_INPUT;
	Delay(1);
	PS2_KBD_SET_CLK_INPUT;
}

/********************************************************************
// Function name	: PS2_KBD_Get_Bit
// Description	    : ¥”PS/2º¸≈Ã…Ë±∏∂¡»°“ª∏ˆBit
// Return type		: char
// Argument         : void
*********************************************************************/
char PS2_KBD_Get_Bit(void)
{
	char clk, bit;
	
	// µ»¥˝ ±÷”œﬂ±‰Œ™µÕµÁ∆Ω
	do
	{
		clk = PS2_KBD_CLK_READ();

	}while(clk);
	
	// ∂¡»° ˝æ›
	bit = PS2_KBD_DATA_READ();
	
	// µ»¥˝ ±÷”œﬂ±‰Œ™∏ﬂµÁ∆Ω
	do
	{
		clk = PS2_KBD_CLK_READ();
	}while(!clk);

	return bit;
}

/********************************************************************
// Function name	: PS2_KBD_Get_Byte
// Description	    : ¥”PS/2º¸≈Ã∂¡»°“ª∏ˆ◊÷Ω⁄
// Return type		: char
// Argument         : void
*********************************************************************/
char PS2_KBD_Get_Byte(void)
{
	int loopcnt;
	char bit, data = 0;
	
	bit = PS2_KBD_Get_Bit(); // ø™ ºŒª
	
	//  ˝æ›Œª	
	for(loopcnt = 0; loopcnt < 8; loopcnt++)
	{
		data = (PS2_KBD_Get_Bit() << loopcnt) | data; 
	}
	
	bit = PS2_KBD_Get_Bit(); // –£—ÈŒª
	bit = PS2_KBD_Get_Bit(); // Ω· ¯Œª
	
	return data;
}

/********************************************************************
// Function name	: PS2_KBD_Handle_Rawcode
// Description	    : ¥¶¿ÌPS2‘≠ º ˝æ›
// Return type		: void
// Argument         : int keyval
*********************************************************************/
void PS2_KBD_Handle_Rawcode(int keyval)
{
	int keysym;

	if (keyval > 0x83) 
	{
		switch (keyval) 
		{
		case KBD_ESCAPEE0:
			ncodes = 2;
			bi = 0;
			break;

		case KBD_ESCAPEE1:
			ncodes = 3;
			bi = 0;
			break;

		case KBD_ACK:
			tx_state = TX_DONE;
			return;

		case KBD_RESEND:
			tx_state = TX_SEND;
			return;

		case KBD_BREAK:
			status |= CODE_BREAK;
			return;

		default:
			return;
		}
	}

	if (ncodes) 
	{
		buffer[bi++] = keyval;
		ncodes -= 1;
		if (ncodes)
			return;
		keysym = K_NONE;
		switch (buffer[0] << 8 | buffer[1]) 
		{
		case ESCE0(0x11): keysym = K_RALT; break;
		case ESCE0(0x14): keysym = K_RCTL; break;
		/*
		 * take care of MS extra keys (actually
		 * 0x7d - 0x7f, but last one is already K_NONE
		 */
		case ESCE0(0x1f): keysym = 124;    break;
		case ESCE0(0x27): keysym = 125;    break;
		case ESCE0(0x2f): keysym = 126;    break;
		case ESCE0(0x4a): keysym = KP_SLH; break;
		case ESCE0(0x5a): keysym = KP_ENT; break;
		case ESCE0(0x69): keysym = K_END;  break;
		case ESCE0(0x6b): keysym = K_LEFT; break;
		case ESCE0(0x6c): keysym = K_HOME; break;
		case ESCE0(0x70): keysym = K_INS;  break;
		case ESCE0(0x71): keysym = K_DEL;  break;
		case ESCE0(0x72): keysym = K_DOWN; break;
		case ESCE0(0x74): keysym = K_RGHT; break;
		case ESCE0(0x75): keysym = K_UP;   break;
		case ESCE0(0x7a): keysym = K_PGDN; break;
		case ESCE0(0x7c): keysym = K_PRNT; break;
		case ESCE0(0x7d): keysym = K_PGUP; break;
		case ESCE1(0x14):
			if (buffer[2] == 0x77)
				keysym = K_BRK;
			break;
		case ESCE0(0x12):		/* ignore escaped shift key */
			status = 0;
			return;
		}
	}else 
	{
		bi = 0;
		keysym = keycode_translate[keyval];
		
		if(!(status & CODE_BREAK)) 
			Uart_Printf("%c", ps2kbd_sysrq_xlate[keysym]);		
	}

	status = 0;
}




