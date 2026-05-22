#include "stm32f10x.h"
#include "delay.h"
#include "sys.h"
#include "pca9685.h"
#include "myiic.h"
u16 addr1; 
 int main(void)
 {	
	  addr1=0X00;
    delay_init();
	 	IIC_Init();
	  pca_reset(addr1);
	for(int n=0;n<20;n++)   
			data_20[n]=0;
		pca_write20(addr1);
		delay_ms(10);
		while(1)
		{			
			for(int n=0;n<16;n++)
			{
					for(int m=0;m<=180;m++)
					{
							pca_setpwm(addr1,n,m);	
							delay_ms(2);						
					}					
			}
			delay_ms(500);	
			for(int n=0;n<16;n++)
			{
					for(int m=0;m<=180;m++)
					{
							pca_setpwm(addr1,15-n,180-m);	
							delay_ms(2);						
					}					
			}		
			delay_ms(500);				
	 }
}
 


