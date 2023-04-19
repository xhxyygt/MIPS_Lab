/*
 * ParIO_ProCtr.c
 *
 *  Created on: 2023年4月18日
 *      Author: lenovo
 */
#include "stdio.h"
#include "xil_io.h"
#include "xgpio_l.h"


char segtable[16] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, 0x88, 0x83, 0xc6, 0xa1, 0x86, 0x8e}; //段码表
char segcode[8] = {0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0}; //段码初始化为零
char poscode[8] = {0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe}; //位码表

int getarray (int *digit,int num){
	int i;
	int dex;
	for(i=0;i<5;i++){
		dex=num%10;
		num/=10;
		digit[i]=dex;
	}
	return 0;
}


int main()
{
	//GPIO IO
    Xil_Out16(XPAR_GPIO_0_BASEADDR + XGPIO_TRI_OFFSET, 0xff);//实现GPIO0 通道1为输入（开关）
    Xil_Out16(XPAR_GPIO_1_BASEADDR + XGPIO_TRI2_OFFSET, 0x0);//实现GPIO1 通道2全为输出（段选）
    Xil_Out16(XPAR_GPIO_1_BASEADDR + XGPIO_TRI_OFFSET, 0x0);//实现GPIO1 通道1为输出（位选）
    Xil_Out16(XPAR_GPIO_2_BASEADDR + XGPIO_TRI_OFFSET, 0x1f);//实现GPIO2 通道1为输出（按键）

    while(1)
    {
    	char button=Xil_In16(XPAR_AXI_GPIO_2_BASEADDR+XGPIO_DATA_OFFSET);//读按键值
    	unsigned short sw=Xil_In16(XPAR_AXI_GPIO_0_BASEADDR+XGPIO_DATA_OFFSET);//读取开关数值
    	for(int i=0; i<8; i++)
    	{
    		Xil_Out8(XPAR_AXI_GPIO_1_BASEADDR+XGPIO_DATA_OFFSET, poscode[i]);
    		Xil_Out8(XPAR_AXI_GPIO_1_BASEADDR+XGPIO_DATA2_OFFSET, segcode[i]);
    	}
    	if(button==0x01)//BNTC读入16位数，将二进制低八位显示到数码管
    	{
    		do{
    			for(int i=0; i<8; i++)
    			{
    				short key = sw & 0xff; //取低8位
    				for(int digit_index=0; digit_index<8; digit_index++)
    					segcode[7-digit_index]=segtable[(key>>(digit_index))&0x1];
    				Xil_Out8(XPAR_AXI_GPIO_1_BASEADDR+XGPIO_DATA_OFFSET, poscode[i]);
    				Xil_Out8(XPAR_AXI_GPIO_1_BASEADDR+XGPIO_DATA2_OFFSET, segcode[i]);
    				for(int j=0; j<10000; j++);
    			}
    			button = Xil_In16(XPAR_AXI_GPIO_2_BASEADDR+XGPIO_DATA_OFFSET); //检测button，发现有按键按下就退出循环
    		}while(button == 0x00);
//    		if(button != 0x01) break;
    	}
    	if(button==0x02)//BTNU读入16位数，将对应的四个十六进制数显示到数码管低4位
    	{
    		do{
    			for(int i=0; i<8; i++)
    			{
    				short key = sw;
    				for(int digit_index=0; digit_index<4; digit_index++)
    					segcode[7-digit_index]=segtable[(key>>(4*digit_index))&0xf];
    				Xil_Out8(XPAR_AXI_GPIO_1_BASEADDR+XGPIO_DATA_OFFSET, poscode[i]);
    				Xil_Out8(XPAR_AXI_GPIO_1_BASEADDR+XGPIO_DATA2_OFFSET, segcode[i]);
    				for(int j=0; j<10000; j++);
    			}
    			button = Xil_In16(XPAR_AXI_GPIO_2_BASEADDR+XGPIO_DATA_OFFSET); //检测button，发现有按键按下就退出循环
    		}while(button == 0x00);
    	}
    	if(button==0x10)//BTND读入16位二进制数，显示对应的5位无符号十进制
    	{
    		do{
    			for(int i=0; i<8; i++)
    			{
    				unsigned short key = sw;
    				int digit[5] = {0};
    				getarray(digit, key);
    				for (int digit_index=0; digit_index<5; digit_index++)
    					segcode[7-digit_index] = segtable[digit[digit_index]];
    				Xil_Out8(XPAR_AXI_GPIO_1_BASEADDR+XGPIO_DATA_OFFSET, poscode[i]);
    				Xil_Out8(XPAR_AXI_GPIO_1_BASEADDR+XGPIO_DATA2_OFFSET, segcode[i]);
    				for(int j=0; j<10000; j++);
    			}
    			button = Xil_In16(XPAR_AXI_GPIO_2_BASEADDR+XGPIO_DATA_OFFSET); //检测button，发现有按键按下就退出循环
    		}while(button == 0x00);
    	}
    	for(int i=0; i<8; i++)
    	{
    		segcode[i] = 0xff; //默认为不显示
    	}
    }
}
