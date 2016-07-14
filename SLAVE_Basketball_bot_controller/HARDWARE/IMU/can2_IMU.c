#include "can2_IMU.h"
#include "led.h"
#include "delay.h"
#include "string.h"
//////////////////////////////////////////////////////////////////////////////////	 
//XM STM32F407���ذ�
//CAN��������	   
//XM ������
//��������:2015/8/30
//�汾��V1.0
//������ҵ��ѧ�赸�����˻��� ����								  
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//CAN��ʼ��                                                                                                             //
//tsjw:����ͬ����Ծʱ�䵥Ԫ. @ref CAN_synchronisation_jump_width   ��Χ: ; CAN_SJW_1tq~ CAN_SJW_4tq                     //
//tbs2:ʱ���2��ʱ�䵥Ԫ.   @ref CAN_time_quantum_in_bit_segment_2 ��Χ:CAN_BS2_1tq~CAN_BS2_8tq;                        //
//tbs1:ʱ���1��ʱ�䵥Ԫ.   @refCAN_time_quantum_in_bit_segment_1  ��Χ: ;	  CAN_BS1_1tq ~CAN_BS1_16tq                 //
//brp :�����ʷ�Ƶ��.��Χ:1~1024;(ʵ��Ҫ��1,Ҳ����1~1024) tq=(brp)*tpclk1                                                //
//������=Fpclk1/((tsjw+tbs1+tbs2+3)*brp);                                                                               //
//mode: @ref CAN_operating_mode ��Χ��CAN_Mode_Normal,��ͨģʽ;CAN_Mode_LoopBack,�ػ�ģʽ;                              //
//Fpclk1��ʱ���ڳ�ʼ����ʱ������Ϊ36M,�������CAN_Normal_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6,CAN_Mode_LoopBack); //
//������Ϊ:42M/((1+6+7)*6)=500Kbps                                                                                    //
//����ֵ:0,��ʼ��OK;                                                                                                    //
//    ����,��ʼ��ʧ��;                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CanRxMsg RxMessage;//����ȫ�ֱ����洢CAN�յ������ݣ�����main.c��ʹ��

__IO u8 Flag1_CAN_Receive=0xff, Flag2_CAN_Receive=0xff; //CAN�жϽ��ձ�־λ���ɹ��Ǳ�־λΪ0
__IO u8 GYPOS_DATA[4]={0};   //�洢�����Ǵ��ص�����

u8 CAN2_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode)
{

  	GPIO_InitTypeDef GPIO_InitStructure; 
	  CAN_InitTypeDef        CAN_InitStructure;
  	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
#if CAN2_RX1_INT_ENABLE 
   	NVIC_InitTypeDef  NVIC_InitStructure;
#endif
    //ʹ�����ʱ��
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//ʹ��PORTAʱ��	                   											 

  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2, ENABLE);//ʹ��CAN2ʱ��	

	  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_5|GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
    GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��PA11,PA12
	
		GPIO_PinAFConfig(GPIOB,GPIO_PinSource5,GPIO_AF_CAN2); //GPIOA11����ΪCAN2
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource6,GPIO_AF_CAN2); //GPIOA12����ΪCAN2
    //��ʼ��GPIO
	  //���Ÿ���ӳ������

  	//CAN��Ԫ����
   	CAN_InitStructure.CAN_TTCM=DISABLE;	//��ʱ�䴥��ͨ��ģʽ   
  	CAN_InitStructure.CAN_ABOM=DISABLE;	//�����Զ����߹���	  
  	CAN_InitStructure.CAN_AWUM=DISABLE;//˯��ģʽͨ����������(���CAN->MCR��SLEEPλ)
  	CAN_InitStructure.CAN_NART=ENABLE;	//��ֹ�����Զ����� 
  	CAN_InitStructure.CAN_RFLM=DISABLE;	//���Ĳ�����,�µĸ��Ǿɵ�  
  	CAN_InitStructure.CAN_TXFP=DISABLE;	//���ȼ��ɱ��ı�ʶ������ 
  	CAN_InitStructure.CAN_Mode=mode;	 //ģʽ���� 
  	CAN_InitStructure.CAN_SJW=tsjw;	//����ͬ����Ծ����(Tsjw)Ϊtsjw+1��ʱ�䵥λ CAN_SJW_1tq~CAN_SJW_4tq
  	CAN_InitStructure.CAN_BS1=tbs1; //Tbs1��ΧCAN_BS1_1tq ~CAN_BS1_16tq
  	CAN_InitStructure.CAN_BS2=tbs2;//Tbs2��ΧCAN_BS2_1tq ~	CAN_BS2_8tq
  	CAN_InitStructure.CAN_Prescaler=brp;  //��Ƶϵ��(Fdiv)Ϊbrp+1	
  	CAN_Init(CAN2, &CAN_InitStructure);   // ��ʼ��CAN2 
    
		//���ù�����
 	  CAN_FilterInitStructure.CAN_FilterNumber=14;	  //������14
  	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; //32λ 
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;////32λID
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32λMASK
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
   	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO1;//������14������FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //���������14
  	CAN_FilterInit(&CAN_FilterInitStructure);//�˲�����ʼ��
		//CAN_ITConfig(CAN2,CAN_IT_FMP0, ENABLE);//
		
#if CAN2_RX1_INT_ENABLE
	
	  CAN_ITConfig(CAN2,CAN_IT_FMP1,ENABLE);//FIFO0��Ϣ�Һ��ж�����.		    
  
  	NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX1_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     // �����ȼ�Ϊ1
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;            // �����ȼ�Ϊ0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif
	return 0;
}   
 
#if CAN2_RX1_INT_ENABLE	//ʹ��RX0�ж�
//CAN�����ж�
int i=0;
char byte0[12];
float this_yaw_angle;
float last_yaw_angle;
float temp_yaw_angle;
float D_last_yaw_angle, D_this_yaw_angle;
int32_t turn_cnt = 0;
 float Yaw_Angle_temp=0;
 float Yaw_Angle_Last=0;   //�˲���

float Gypo_Offset = 0.0;
float Yaw_Angle = 0.0;
	
//�жϷ�����	
void CAN2_RX1_IRQHandler(void)
{
	CAN_Receive(CAN2, 1 ,&RxMessage);
	//LED2=0;
	if(RxMessage.StdId==0X0401)
	 {
		 temp_yaw_angle = (int32_t)(RxMessage.Data[0]<<24)|(int32_t)(RxMessage.Data[1]<<16) 
            |(int32_t)(RxMessage.Data[2]<<8)|(int32_t)(RxMessage.Data[3]);
          
     this_yaw_angle = ((float)temp_yaw_angle*0.01f);
		 
		 if(this_yaw_angle>36000||this_yaw_angle<-36000)
		 {
			 this_yaw_angle =  Yaw_Angle_temp;
		 }
					
		 D_this_yaw_angle=(this_yaw_angle -Yaw_Angle_temp);			
		 if(D_this_yaw_angle > 500||D_this_yaw_angle<-500)
		 {
			 D_this_yaw_angle=D_last_yaw_angle;
		 }
						
		 D_last_yaw_angle=D_this_yaw_angle;
		 Yaw_Angle_temp  = Yaw_Angle_temp + D_this_yaw_angle;//���Ƕ�ֵת��Ϊ��Ȼ����
						
//					last_yaw_angle = this_yaw_angle;
//						if(Yaw_Angle_temp<2000||Yaw_Angle_temp>-2000)
//						{
//							Yaw_Angle_Last=Yaw_Angle_temp;
//							 memcpy(GYPOS_DATA,&Yaw_Angle_Last,4);
//		       printf("%f  \n",Yaw_Angle_Last);
//						}
//						else 
//						{
		 memcpy(GYPOS_DATA,&Yaw_Angle_temp,4);
		 
		 Yaw_Angle=Yaw_Angle_temp-Gypo_Offset;
//		 printf(" this %6f  Yaw%f \r\n", this_yaw_angle, Yaw_Angle_temp);
//	 }
						
	 } 
	if(RxMessage.Data[1]==0x01&&RxMessage.Data[2]==0xA1)
	{
			byte0[0] = RxMessage.Data[4];  // ��CAN_RCE[4]	λ���ֽ�
			byte0[1] = RxMessage.Data[5];
			byte0[2] = RxMessage.Data[6];
			byte0[3] = RxMessage.Data[7];
	}	
	if(RxMessage.Data[1]==0x02&&RxMessage.Data[2]==0xA1)
	{
			byte0[4] = RxMessage.Data[4];  // ��CAN_RCE[4]	λ���ֽ�
			byte0[5] = RxMessage.Data[5];
			byte0[6] = RxMessage.Data[6];
			byte0[7] = RxMessage.Data[7];
	}	
	if(RxMessage.Data[1]==0x03&&RxMessage.Data[2]==0xA1)
	{
			byte0[8] = RxMessage.Data[4];  // ��CAN_RCE[4]	λ���ֽ�
			byte0[9] = RxMessage.Data[5];
			byte0[10] = RxMessage.Data[6];
			byte0[11] = RxMessage.Data[7];
	}	
	if(RxMessage.Data[0]==0x06&&RxMessage.Data[2]==0x99)
		Flag1_CAN_Receive=0;
	else if(RxMessage.Data[0]==0x08&&(RxMessage.Data[2]==0x9B||RxMessage.Data[2]==0x9E||RxMessage.Data[2]==0xA1))
	{
		Flag2_CAN_Receive=0;
		//LED1=0;
	}
	else
	{
		Flag1_CAN_Receive=0XFF;
		Flag2_CAN_Receive=0xFF;
	}
}
#endif
//can����һ������(�̶���ʽ:IDΪ0X12,��׼֡,����֡)	
//ID :can������ID    
//msg:����ָ��,���Ϊ8���ֽ�.
//����ֵ:0,�ɹ�;
//����,ʧ��;
u8 CAN2_Send_Msg(u8* msg,u8 ID)
{	
	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=ID;	 // ��׼��ʶ��Ϊ0
  TxMessage.ExtId=ID;	 // ������չ��ʾ����29λ��
  TxMessage.IDE=0;		  // ʹ����չ��ʶ��
  TxMessage.RTR=0;		  // ��Ϣ����Ϊ����֡��һ֡8λ
  TxMessage.DLC=8;							 // ������֡��Ϣ
  for(i=0;i<8;i++)
  TxMessage.Data[i]=msg[i];				 // ��һ֡��Ϣ          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
	//LED1=0;
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�ȴ����ͽ���
  if(i>=0XFFF)
	{
		//LED2=0;
		return 1;
	}
  return 0;		
}
