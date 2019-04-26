/****************************************************************************/
/*文件名称： LEDSEG7.C                                                      */
/*实验现象： 数码管依次显示出0、1，2、……9、a、b、C、d、E、F               */
/****************************************************************************/
#define U8 unsigned char
unsigned char seg7table[16] = {
    /* 0       1       2       3       4       5       6      7*/

    0xc0,   0xf9,   0xa4,   0xb0,   0x99,   0x92,   0x82,   0xf8,

    /* 8       9      A        B       C       D       E      F*/
    0x80,   0x90,   0x88,   0x83,   0xc6,   0xa1,   0x86,   0x8e,
};
void Delay(int time);
/****************************************************************************/
/* 函数说明: JXARM9-2410 7段构共阳数码管测试                                     */
/* 功能描述: 依次在7段数码管上显示0123456789ABCDEF                         */
/* 返回代码: 无                                                             */
/* 参数说明: 无                                                             */
/****************************************************************************/

void Test_Seg7(void) {    
	int i; 	

	*((U8*) 0x20007000) = 0x00;	
	 
	for( ; ; )	{
		/* 数码管从0到F依次将字符显示出来 */
	    for(i=0;i<0x10;i++)		{
			/* 查表并输出数据 */
	    	*((U8*) 0x20006000) = seg7table[i];    
	    	Delay (10000);   	   
   		}
		
		/* 数码管从F到0依次将字符显示出来 */
		for(i=0xf;i>=0x0;i--)		{
	   		/* 查表并输出数据 */
	   		*((U8*) 0x20006000) = seg7table[i];    
	   		Delay (10000);		
	 	}
	 }
}

/****************************************************************************/
/* Function name : 循环延时子程序                                           */
/* Description : 循环 'time' 次                                             */
/* Return type ：void                                                       */
/* Argument      : 循环延时计数器                                           */
/****************************************************************************/
void Delay(int time) {
    int i;
	int delayLoopCount=1000;



 
      for(;time>0;time--);
		for(i=0;i<delayLoopCount;i++);
}
