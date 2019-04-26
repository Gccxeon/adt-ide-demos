/*
* GPSRF1006 Message Format
*
*   UTC : Universal time coordinated 
*
*   $IDMSG,D1,D2,D3,D4,.................,Dn*CS[CR][LF]
*
*   GGA,hhmmss,llll.lll,a,nnnnn.nnn,b,t,uu,v.v,w.w,M,x.x,M,y.y,zzzz*hh
*   VTG,x.x,T,x.x,M,x.x,N,x.x,K*hh
*
*   GGA:
*        1      UTC of position.
*        2,3    Latitude, N(North) or S(South).
*        4,5    Longitude, E(East) or W(West).
*        6      GPS Quality Indicator: 0=No GPS, 1=GPS, 2=DGPS.
*        7      Number of satellites in use. 
*        8      Horizontal Dilution of Precision (HDOP).
*        9,10   Antenna altitude in meters, M=Meters.
*        11,12  Geoidal separation in meters, M=Meters.
*        13     Age of Diferential GPS Data.
*        14     Differential Reference Station ID (0000 to 1023).
*        hh     Checksum.
*        
*   VTG:
*        1      Track made good in degrees true.
*        2      Track made good in degrees magnetic.
*        3,4    Speed over the ground (SOG) in knots.
*        5,6    Speed over the ground (SOG) in kilometer per hour.
*        hh     Checksum.
*/
#include "string.h"

#include "gpslib.h"

/*******************************************************************************
*
* defines.
*
******************************************************************************/

#define LEN_MSG_GPS     80   // Numbers of maximum message char
#define MAX_PARAMS         20   // Numbers of maximum parameters

/*******************************************************************************
*
* Variables locales.
*
******************************************************************************/

static UInt8 bufRx[LEN_MSG_GPS];   // Buffer recepcion.
static UInt8 numRx;                 // Numero bytes recibidos. 

#define RX_WAIT_START     0       //   Esperando inicio comando.
#define RX_RECEIVE_DATA   1       //   Recibiendo mensaje.
#define RX_CHECKSUM_H     2       //   Espera checksum high.
#define RX_CHECKSUM_L     3       //   Espera checksum low.
static UInt8 stateRx = RX_WAIT_START;               // Estado recepcion mensajes GPS:


/*******************************************************************************
*
* Declaracion funciones locales.
*
******************************************************************************/

static void ProcessMsgGPS(GPSINFO * pinfo, UInt8 *pMsg, UInt8 len);

static void GetLatitude(GPSINFO * pinfo, UInt8 *param);
static void GetLongitude(GPSINFO * pinfo, UInt8 *param);
static void GetTime(GPSINFO * pinfo, UInt8 *param);

static UInt8 CharHToI(UInt8 h);

//******************************************************************************
//  FUNCION    : GPSInit
//  DESCRIPCION: init GPS.
//  PARAMETROS : void.
//  DEVUELTO   : void.
//******************************************************************************
void GPSInit(void)
{
	// init variables.
	stateRx = RX_WAIT_START;
	numRx = 0;
}
//******************************************************************************
//  FUNCION    : GPSReceive
//  DESCRIPCION: get the message of GPS string.
//  PARAMETROS : pStreamIn - Buffer point.
//               len       - Numero de bytes recibidos.
//  DEVUELTO   : Ninguno.
//******************************************************************************
void GPSReceive(GPSINFO * pinfo, UInt8* pStreamIn, UInt32 len)
{
	static UInt8 chkSum, chk;    // Checksum.
	UInt8 i, c;
	
	pinfo->bIsGPGGA = 0;
	
	for (i = 0; i < len; i++)
	{
		c = pStreamIn[i];
		
		if (stateRx == RX_WAIT_START)
		{
			if (c == '$')
			{
				stateRx = RX_RECEIVE_DATA;
				chkSum = 0;
			}
		}		
		else if (stateRx == RX_RECEIVE_DATA)
		{
			if (c == '*')
			{
				stateRx = RX_CHECKSUM_H;
			}
			else
			{
				bufRx[numRx++] = c;
				
				// Calcular checksum.
				if (numRx == 1)
					chkSum = c;
				else
					chkSum ^= c;
				
				// complete.
				if (numRx > LEN_MSG_GPS)
				{
					numRx = 0;
					stateRx = RX_WAIT_START;
				}
			}
		}		
		else if (stateRx == RX_CHECKSUM_H)
		{
			chk = CharHToI(c) << 4;
			stateRx = RX_CHECKSUM_L;
		}		
		else if (stateRx == RX_CHECKSUM_L)
		{
			chk += CharHToI(c);
			
			// check sum , process if ok.
			if (chkSum == chk)      
				ProcessMsgGPS(pinfo, bufRx, numRx);
			
			numRx = 0;
			stateRx = RX_WAIT_START;
		}			
	}
}

//******************************************************************************
//  FUNCION    : ProcessMsgGPS
//  DESCRIPCION: Process the message of GPS.
//  PARAMETROS : pMsg - message point.
//               len  - length of message.
//  DEVUELTO   : Ninguno.
//******************************************************************************
static void ProcessMsgGPS(GPSINFO * pinfo, UInt8 *pMsg, UInt8 len)
{
	UInt8  buf[LEN_MSG_GPS+2];  // Buffer.
	UInt8* params[MAX_PARAMS];   // parameters.
	UInt8  numParam;             // numbers of parameters.
	UInt8* pParam;               // pt.
	UInt8* pComma;
	
	// --- search identification "GP". ---
	if (strstr(pMsg, "GP") == 0)
		return;
	
	// --- parser parameters. ---
	// search all the ',' to replace with '\0'
	strcpy(buf, pMsg+6);
	strcat(buf, ",*");  
	numParam = 0;
	pParam = buf;
	while (*pParam != '*') 
	{
		pComma = strchr(pParam, ',');
		params[numParam++] = pParam;
		*pComma = 0;
		pParam = pComma + 1;
	}
	
	// --- process parameters. ---
	if (strstr(pMsg+2, "GGA") != 0)
	{
		// GPGGA
		GetTime(pinfo, params[0]);                    // get UTC time
		GetLatitude(pinfo, params[1]);                // get latitude
		pinfo->latNS = *params[2];                    // latitude : N or S
		GetLongitude(pinfo, params[3]);               // get longitude
		pinfo->lgtEW = *params[4];                    // longitude : E or W
		pinfo->satellites = atoi(params[6]);          // Number of satellites in use
		pinfo->altitude = atoi(params[8]);            // altitude
		pinfo->altUnit = *params[9];                  // M=Meters
		
		/* 设置接收到GPGGA标记 */
		pinfo->bIsGPGGA = 1;
	}
	else if (strstr(pMsg+2, "VTG") != 0)
	{
	}
}


//******************************************************************************
//  FUNCION    : GetLatitude
//  DESCRIPCION: Extract latitud from string
//
//               globales: latitude.
//
//  PARAMETROS : param - string.
//  DEVUELTO   : void.
//******************************************************************************
static void GetLatitude(GPSINFO * pinfo, UInt8 *param)
{
	UInt8 buf[20];
	int   dd, mm, mmmm;
	
	// dd
	memcpy(buf, param, 2);
	buf[2] = 0;
	dd = atoi(buf);
	
	// mm.mmmm
	memcpy(buf, param+2, 2);
	buf[2] = 0;
	mm = atoi(buf);
	
	// mmmm
	memcpy(buf, param+5, 4);
	buf[4] = 0;
	mmmm = atoi(buf);
	pinfo->latitude = dd + mm* 0.01 + mmmm * 0.01 * 0.01 * 0.01;
}


//******************************************************************************
//  FUNCION    : GetLongitude
//  DESCRIPCION: Extract longtitude from string
//
//               globales: longitud.
//
//  PARAMETROS : param - string.
//  DEVUELTO   : void.
//******************************************************************************
static void GetLongitude(GPSINFO * pinfo, UInt8 *param)
{
	UInt8 buf[20];
	int   dd, mm, mmmm;
	
	if(strlen(param)==10) file://经度超过90度(如东经125度)
	{
		// ddd
		memcpy(buf, param, 3);
		buf[3] = 0;
		dd = atoi(buf);
		
		// mm
		memcpy(buf, param+3, 2);
		buf[2] = 0;
		mm = atoi(buf);
		
		// mmmm
		memcpy(buf, param+6, 4);
		buf[4] = 0;
		mmmm = atoi(buf);
		pinfo->longitud = dd + mm* 0.01 + mmmm * 0.01 * 0.01 * 0.01;
	}
	if(strlen(param)==9) //经度未超过90度(如东经89度)
	{
		// dd
		memcpy(buf, param, 2);
		buf[2] = 0;
		dd = atoi(buf);
		
		// mm.mmmm
		memcpy(buf, param+2, 2);
		buf[2] = 0;
		mm = atoi(buf);

		// mmmm
		memcpy(buf, param+5, 4);
		buf[4] = 0;
		mmmm = atoi(buf);
		pinfo->longitud = dd + mm* 0.01 + mmmm * 0.01 * 0.01 * 0.01;
	}
}


//******************************************************************************
//  FUNCION    : GetTime
//  DESCRIPCION: extract the UCT time.
//
//               Variable: hour, min, sec, secFrac.
//
//  PARAMETROS : param - string.
//  DEVUELTO   : void.
//******************************************************************************
static void GetTime(GPSINFO * pinfo, UInt8 *param)
{
	UInt8 buf[20];
	
	// hours
	memcpy(buf, param, 2);
	buf[2] = 0;
	pinfo->hour = atoi(buf);        // UTC Time
	pinfo->bjhour = (pinfo->hour + 8) % 24;  // BeiJing Time
	
	// minutes
	memcpy(buf, param+2, 2);
	buf[2] = 0;
	pinfo->min = atoi(buf);
	
	// Seconds
	memcpy(buf, param+4, 2);
	buf[2] = 0;
	pinfo->sec = atoi(buf);
	
	// ms
	memcpy(buf, param+7, 2);
	strcat(buf, "00");
	buf[2] = 0;
	pinfo->secFrac = atoi(buf);
}


//******************************************************************************
//  FUNCION    : CharHToI
//  DESCRIPCION: convert a char from hexadecimal a decimal.
//  PARAMETROS : c - Character hexadecimal.
//  DEVUELTO   : decimal.
//******************************************************************************
static UInt8 CharHToI(UInt8 h)
{
	if ((h >= '0') && (h <= '9'))
		return (h - '0');
	else if ((h >= 'A') && (h <= 'F'))
		return (h - 'A' + 10);
	else
		return 0;
}
