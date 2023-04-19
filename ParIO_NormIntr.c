/*
 * ParIO_NormIntr.c
 *
 *  Created on: 2023��4��18��
 *      Author: lenovo
 */

#include "stdio.h"
#include "xil_io.h"
#include "xgpio_l.h"
#include "xintc_l.h"
#include "xtmrctr.h"

#define RESET_VALUE 100000-2 //0.001s

void My_ISR()__attribute__((interrupt_handler));
//void switch_handle();
void button_handle(); //�а����ж�ʱ���뿪��״̬
void timer_handle(); //�ж�ʱ���ж�ʱˢ���������ʾ

unsigned short swin; //����Ŀ�����ֵ
short pos = 0; //�����ɨ�����
char button=0; //����ֵ

char segtable[16] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, 0x88, 0x83, 0xc6, 0xa1, 0x86, 0x8e}; //�����
char segcode[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; //�����ʼ��Ϊ��
char poscode[8] = {0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe}; //λ���

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
    Xil_Out16(XPAR_GPIO_0_BASEADDR + XGPIO_TRI_OFFSET, 0xff);//ʵ��GPIO0 ͨ��1Ϊ���루���أ�
    Xil_Out16(XPAR_GPIO_1_BASEADDR + XGPIO_TRI2_OFFSET, 0x0);//ʵ��GPIO1 ͨ��2ȫΪ�������ѡ��
    Xil_Out16(XPAR_GPIO_1_BASEADDR + XGPIO_TRI_OFFSET, 0x0);//ʵ��GPIO1 ͨ��1Ϊ�����λѡ��
    Xil_Out16(XPAR_GPIO_2_BASEADDR + XGPIO_TRI_OFFSET, 0x1f);//ʵ��GPIO2 ͨ��1Ϊ�����������

    //GPIO Enable Interrupt
    Xil_Out32(XPAR_GPIO_2_BASEADDR + XGPIO_ISR_OFFSET, XGPIO_IR_CH1_MASK); //���ͨ��1���ж�״̬
    Xil_Out32(XPAR_GPIO_2_BASEADDR + XGPIO_IER_OFFSET, XGPIO_IR_CH1_MASK); //ʹ��ͨ��1�ж�
    Xil_Out32(XPAR_GPIO_2_BASEADDR + XGPIO_GIE_OFFSET, XGPIO_GIE_GINTR_ENABLE_MASK); //ʹ��GPIO���ж����

    //Timer T0
    Xil_Out32(XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET,
    	Xil_In32((XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET)&~XTC_CSR_ENABLE_TMR_MASK));//дTCSR��ֹͣ��ʱ��
    Xil_Out32(XPAR_AXI_TIMER_0_BASEADDR+XTC_TLR_OFFSET,RESET_VALUE);//дTLRԤ�ö�ʱ����ֵ
    Xil_Out32(XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET,
    	Xil_In32((XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET)|XTC_CSR_LOAD_MASK));//װ�ؼ�������ֵ��дTCSRʹ��LOADx=1

    Xil_Out32(XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET,
    	Xil_In32((XPAR_AXI_TIMER_0_BASEADDR+XTC_TCSR_OFFSET)&XTC_CSR_LOAD_MASK)\
    	|XTC_CSR_ENABLE_TMR_MASK|XTC_CSR_AUTO_RELOAD_MASK|XTC_CSR_ENABLE_INT_MASK|XTC_CSR_DOWN_COUNT_MASK);
    	//LOADx=0����װ�أ���ENTx=1���ж�ʹ�ܣ���ARHT=1���Զ��ظ�װ�أ���
        //����T0����ģʽ������������������λ����
        //��ʼ��ʱ���У��Զ���ȡ�������жϣ�������

    //INTC
    Xil_Out32(XPAR_INTC_0_BASEADDR + XIN_IAR_OFFSET,
    		XPAR_AXI_GPIO_2_IP2INTC_IRPT_MASK
			|XPAR_AXI_TIMER_0_INTERRUPT_MASK); //���GPIO2��Timerԭ�ж�״̬
    Xil_Out32(XPAR_INTC_0_BASEADDR + XIN_IER_OFFSET,
    		XPAR_AXI_GPIO_2_IP2INTC_IRPT_MASK
			|XPAR_AXI_TIMER_0_INTERRUPT_MASK); //������λ��Ӧ���ж�����
    Xil_Out32(XPAR_INTC_0_BASEADDR + XIN_MER_OFFSET,
    		XIN_INT_MASTER_ENABLE_MASK
			|XIN_INT_HARDWARE_ENABLE_MASK); //ʹ�����жϣ���дHIE(D1)��ME(D0)Ϊ1ʹ��Ӳ���жϺ���������ж������ź�

    microblaze_enable_interrupts();

    return 0;
}

void My_ISR()
{
	int status;
	status=Xil_In32(XPAR_AXI_INTC_0_BASEADDR+XIN_ISR_OFFSET);//�����ж�״̬�Ĵ���
	if((status&XPAR_AXI_GPIO_2_IP2INTC_IRPT_MASK)==XPAR_AXI_GPIO_2_IP2INTC_IRPT_MASK) //GPIO2���ж�
		button_handle(); //������״̬
	else if((status&XPAR_AXI_TIMER_0_INTERRUPT_MASK) == XPAR_AXI_TIMER_0_INTERRUPT_MASK) //Timer���ж�
		timer_handle(); //�ڶ�ʱ�������ж�ʱ���������
	Xil_Out32(XPAR_AXI_INTC_0_BASEADDR+XIN_IAR_OFFSET,status); //дIAR ����ж�״̬�Ĵ���ISR
}

void button_handle()
{
	swin = Xil_In16(XPAR_GPIO_0_BASEADDR + XGPIO_DATA_OFFSET);//��ȡ����״̬
	short tmp=Xil_In16(XPAR_AXI_GPIO_2_BASEADDR+XGPIO_DATA_OFFSET);//������ֵ
	button = tmp ? tmp : button; //button����������ֵ����ֹ�ɿ������������ж�ʹbuttonΪ0��
//	xil_printf("button %x\n", button);
//	Xil_Out16(XPAR_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET,swin);//led�������״̬
	Xil_Out32(XPAR_GPIO_2_BASEADDR+XGPIO_ISR_OFFSET,Xil_In32(XPAR_AXI_GPIO_2_BASEADDR+XGPIO_ISR_OFFSET));
	//GPIO��IPISR�Ĵ���������ʾ��״̬��д1�������״̬�����൱��INTC��ISR��IARR��
	//IPISR�����Զ�����������Ҫ�ֶ�д������������������жϣ�
}

void timer_handle()
{
//	xil_printf("sw: %d\n", swin);
	/*�����ѡ���룬Ĭ��ȫ������ܶ�����ʾ*/
	for(int i=0; i<8; i++)
	{
		segcode[i] = 0xff; //Ĭ��Ϊ����ʾ
	}
	for(int i=0; i<8; i++)
	{
		Xil_Out8(XPAR_AXI_GPIO_1_BASEADDR+XGPIO_DATA_OFFSET, poscode[i]);
		Xil_Out8(XPAR_AXI_GPIO_1_BASEADDR+XGPIO_DATA2_OFFSET, segcode[i]);
	}

	if(button==0x01)//BNTC����16λ�����������ƵͰ�λ��ʾ�������
	{
		Xil_Out8(XPAR_GPIO_1_BASEADDR + XGPIO_DATA_OFFSET,poscode[7-pos]);
		Xil_Out8(XPAR_GPIO_1_BASEADDR + XGPIO_DATA2_OFFSET,segtable[(swin>>pos)&0x1]);
		pos++;
		if(pos>=8) pos=0; //�˴�������==���ж���������ֹpos���ִ���8���±ߵĴ���4�ȣ�������ת��0��һֱ�ӵ��³����ܷ�
	}
	if(button==0x02)//BTNU����16λ��������Ӧ���ĸ�ʮ����������ʾ������ܵ�4λ
	{
		Xil_Out8(XPAR_GPIO_1_BASEADDR + XGPIO_DATA_OFFSET,poscode[7-pos]);
		Xil_Out8(XPAR_GPIO_1_BASEADDR + XGPIO_DATA2_OFFSET,segtable[(swin>>4*pos)&0xf]);
		pos++;
		if(pos>=4) pos=0;
	}
	if(button==0x10)//BTND����16λ������������ʾ��Ӧ��5λ�޷���ʮ����
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
