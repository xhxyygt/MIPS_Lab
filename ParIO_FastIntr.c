/*
 * ParIO_FastIntr.c
 *
 *  Created on: 2023年4月19日
 *      Author: lenovo
 */
#include "stdio.h"
#include "xil_io.h"
#include "xgpio_l.h"
#include "xintc_l.h"
#include "xtmrctr.h"

#define RESET_VALUE 100000-2 //0.001s

//void switch_handle();
void button_handle()__attribute__((fast_interrupt)); //有按键中断时读入开关状态
void timer_handle()__attribute__((fast_interrupt)); //有定时器中断时刷新数码管显示

unsigned short swin; //输入的开关数值
short pos = 0; //数码管扫描参数
char button=0; //按键值

char segtable[16] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, 0x88, 0x83, 0xc6, 0xa1, 0x86, 0x8e}; //段码表
char segcode[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; //段码初始化为零
char poscode[8] = {0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe}; //位码表

int main()
{
	//GPIO IO
    Xil_Out16(XPAR_GPIO_0_BASEADDR + XGPIO_TRI_OFFSET, 0xff);//实现GPIO0 通道1为输入（开关）
    Xil_Out16(XPAR_GPIO_1_BASEADDR + XGPIO_TRI2_OFFSET, 0x0);//实现GPIO1 通道2全为输出（段选）
    Xil_Out16(XPAR_GPIO_1_BASEADDR + XGPIO_TRI_OFFSET, 0x0);//实现GPIO1 通道1为输出（位选）
    Xil_Out16(XPAR_GPIO_2_BASEADDR + XGPIO_TRI_OFFSET, 0x1f);//实现GPIO2 通道1为输出（按键）

    //GPIO Enable Interrupt
    Xil_Out32(XPAR_GPIO_2_BASEADDR + XGPIO_ISR_OFFSET, XGPIO_IR_CH1_MASK); //清除通道1的中断状态
    Xil_Out32(XPAR_GPIO_2_BASEADDR + XGPIO_IER_OFFSET, XGPIO_IR_CH1_MASK); //使能通道1中断
    Xil_Out32(XPAR_GPIO_2_BASEADDR + XGPIO_GIE_OFFSET, XGPIO_GIE_GINTR_ENABLE_MASK); //使能GPIO的中断输出

    //Timer T0
    Xil_Out32(XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET,
    	Xil_In32((XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET)&~XTC_CSR_ENABLE_TMR_MASK));//写TCSR，停止定时器
    Xil_Out32(XPAR_AXI_TIMER_0_BASEADDR+XTC_TLR_OFFSET,RESET_VALUE);//写TLR预置定时器初值
    Xil_Out32(XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET,
    	Xil_In32((XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET)|XTC_CSR_LOAD_MASK));//装载计数器初值，写TCSR使得LOADx=1

    Xil_Out32(XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET,
    	Xil_In32((XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET)&XTC_CSR_LOAD_MASK)\
    	|XTC_CSR_ENABLE_TMR_MASK|XTC_CSR_AUTO_RELOAD_MASK|XTC_CSR_ENABLE_INT_MASK|XTC_CSR_DOWN_COUNT_MASK);
    	//LOADx=0（不装载），ENTx=1（中断使能），ARHT=1（自动重复装载），
        //设置T0工作模式并启动计数器，其余位不变
        //开始计时运行，自动获取，允许中断，减计数

    //INTC
    Xil_Out32(XPAR_INTC_0_BASEADDR + XIN_IAR_OFFSET,
    		XPAR_AXI_GPIO_2_IP2INTC_IRPT_MASK
			|XPAR_AXI_TIMER_0_INTERRUPT_MASK); //IAR 清除GPIO2和Timer原中断状态
    Xil_Out32(XPAR_INTC_0_BASEADDR + XIN_IMR_OFFSET,
        	XPAR_AXI_GPIO_2_IP2INTC_IRPT_MASK
    		|XPAR_AXI_TIMER_0_INTERRUPT_MASK); //IMR 配置两个通道中断方式为快速中断
    Xil_Out32(XPAR_INTC_0_BASEADDR + XIN_IER_OFFSET,
    		XPAR_AXI_GPIO_2_IP2INTC_IRPT_MASK
			|XPAR_AXI_TIMER_0_INTERRUPT_MASK); //IER 开这两位对应的中断引脚
    Xil_Out32(XPAR_INTC_0_BASEADDR + XIN_MER_OFFSET,
    		XIN_INT_MASTER_ENABLE_MASK
			|XIN_INT_HARDWARE_ENABLE_MASK); //MER 使能总中断，即写HIE(D1)和ME(D0)为1使能硬件中断和允许产生中断请求信号
    Xil_Out32(XPAR_INTC_0_BASEADDR + XIN_IVAR_OFFSET +
    		4 * XPAR_INTC_0_GPIO_2_VEC_ID, (int)button_handle); //IVAR 填中断向量表
    Xil_Out32(XPAR_INTC_0_BASEADDR + XIN_IVAR_OFFSET +
    		4 * XPAR_INTC_0_TMRCTR_0_VEC_ID, (int)timer_handle); //IVAR 填中断向量表
    microblaze_enable_interrupts();

    return 0;
}

void button_handle()
{
	swin = Xil_In16(XPAR_GPIO_0_BASEADDR + XGPIO_DATA_OFFSET);//读取开关状态
	short tmp=Xil_In16(XPAR_AXI_GPIO_2_BASEADDR+XGPIO_DATA_OFFSET);//读按键值
	button = tmp ? tmp : button; //button仅读入非零的值（防止松开按键产生的中断使button为0）
//	xil_printf("button %x\n", button);
//	Xil_Out16(XPAR_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET,swin);//led输出开关状态
	Xil_Out32(XPAR_GPIO_2_BASEADDR+XGPIO_ISR_OFFSET,Xil_In32(XPAR_AXI_GPIO_2_BASEADDR+XGPIO_ISR_OFFSET)); //清中断
}

void timer_handle()
{
//	xil_printf("sw: %d\n", swin);
	/*清除段选代码，默认全部数码管都不显示*/
	for(int i=0; i<8; i++)
	{
		segcode[i] = 0xff; //默认为不显示
	}
	for(int i=0; i<8; i++)
	{
		Xil_Out8(XPAR_AXI_GPIO_1_BASEADDR+XGPIO_DATA_OFFSET, poscode[i]);
		Xil_Out8(XPAR_AXI_GPIO_1_BASEADDR+XGPIO_DATA2_OFFSET, segcode[i]);
	}

	if(button==0x01)//BNTC读入16位数，将二进制低八位显示到数码管
	{
		Xil_Out8(XPAR_GPIO_1_BASEADDR + XGPIO_DATA_OFFSET,poscode[7-pos]);
		Xil_Out8(XPAR_GPIO_1_BASEADDR + XGPIO_DATA2_OFFSET,segtable[(swin>>pos)&0x1]);
		pos++;
		if(pos>=8) pos=0;
	}
	if(button==0x02)//BTNU读入16位数，将对应的四个十六进制数显示到数码管低4位
	{
		Xil_Out8(XPAR_GPIO_1_BASEADDR + XGPIO_DATA_OFFSET,poscode[7-pos]);
		Xil_Out8(XPAR_GPIO_1_BASEADDR + XGPIO_DATA2_OFFSET,segtable[(swin>>4*pos)&0xf]);
		pos++;
		if(pos>=4) pos=0; //不用==防止pos>4导致不能清零一直++
	}
	if(button==0x10)//BTND读入16位二进制数，显示对应的5位无符号十进制
	{
		unsigned short tmp = swin;
		for(int i = 0; i < pos; i++)
		{
			tmp /= 10;
		}
		Xil_Out8(XPAR_GPIO_1_BASEADDR + XGPIO_DATA_OFFSET,poscode[7-pos]);
		Xil_Out8(XPAR_GPIO_1_BASEADDR + XGPIO_DATA2_OFFSET,segtable[tmp%10]);
		pos++;
		if(pos>=5) pos=0;
	}
	Xil_Out32(XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET, Xil_In32(XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET));
}
