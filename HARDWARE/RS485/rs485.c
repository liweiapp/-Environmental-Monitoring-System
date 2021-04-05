#include "sys.h"
#include "rs485.h"
#include "delay.h"



#if EN_USART2_RX   		//���ʹ���˽���
//���ջ�����
u8 RS485_RX_BUF[64];  	//���ջ���,���64���ֽ�.
//���յ������ݳ���
u8 RS485_RX_CNT = 0;
void USART2_IRQHandler(void)
{
    u8 res;

    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)//���յ�����
    {
        res = USART_ReceiveData(USART2); //;��ȡ���յ�������USART2->DR

        if(RS485_RX_CNT < 64)
        {
            RS485_RX_BUF[RS485_RX_CNT] = res;		//��¼���յ���ֵ
            RS485_RX_CNT++;						//������������1
        }
    }
}
#endif
//��ʼ��IO ����2
//bound:������
void RS485_Init(u32 bound)
{

    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); //ʹ��GPIOAʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); //ʹ��USART2ʱ��

    //����2���Ÿ���ӳ��
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2); //GPIOA2����ΪUSART2
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2); //GPIOA3����ΪUSART2

    //USART2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3; //GPIOA2��GPIOA3
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//�ٶ�100MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
    GPIO_Init(GPIOA, &GPIO_InitStructure); //��ʼ��PA2��PA3

    //PG8���������485ģʽ����
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; //GPIOG8
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//���
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//�ٶ�100MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //�������
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
    GPIO_Init(GPIOG, &GPIO_InitStructure); //��ʼ��PG8


    //USART2 ��ʼ������
    USART_InitStructure.USART_BaudRate = bound;//����������
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
    USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
    USART_Init(USART2, &USART_InitStructure); //��ʼ������2

    USART_Cmd(USART2, ENABLE);  //ʹ�ܴ��� 2

    USART_ClearFlag(USART2, USART_FLAG_TC);

    #if EN_USART2_RX
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//���������ж�

    //Usart2 NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; //��ռ���ȼ�3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
    NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����

    #endif

    RS485_TX_EN = 0;				//Ĭ��Ϊ����ģʽ
}

//RS485����len���ֽ�.
//buf:�������׵�ַ
//len:���͵��ֽ���(Ϊ�˺ͱ�����Ľ���ƥ��,���ｨ�鲻Ҫ����64���ֽ�)
void RS485_Send_Data(u8 *buf, u8 len)
{
    u8 t;
    RS485_TX_EN = 1;			//����Ϊ����ģʽ

    for(t = 0; t < len; t++)		//ѭ����������
    {
        while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET); //�ȴ����ͽ���

        USART_SendData(USART2, buf[t]); //��������
    }

    while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET); //�ȴ����ͽ���

    RS485_RX_CNT = 0;
    RS485_TX_EN = 0;				//����Ϊ����ģʽ
}
//RS485��ѯ���յ�������
//buf:���ջ����׵�ַ
//len:���������ݳ���
void RS485_Receive_Data(u8 *buf, u8 *len)
{
    u8 rxlen = RS485_RX_CNT;
    u8 i = 0;
    *len = 0;				//Ĭ��Ϊ0
    delay_ms(10);		//�ȴ�10ms,��������10msû�н��յ�һ������,����Ϊ���ս���

    if(rxlen == RS485_RX_CNT && rxlen) //���յ�������,�ҽ��������
    {
        for(i = 0; i < rxlen; i++)
        {
            buf[i] = RS485_RX_BUF[i];
        }

        *len = RS485_RX_CNT;	//��¼�������ݳ���
        RS485_RX_CNT = 0;		//����
    }
}



/************
crcУ��
*************/
u16 CRC_CHECK(u8 *buf, u8 CRC_CNT)
{
    u8 CRC_Temp;
    u8 i, j;
    CRC_Temp = 0xffff;

    for (i = 0; i < CRC_CNT; i++)
    {
        CRC_Temp ^= buf[i];

        for (j = 0; j < 8; j++)
        {
            if (CRC_Temp & 0x01)
                CRC_Temp = (CRC_Temp >> 1 ) ^ 0xa001;
            else
                CRC_Temp = CRC_Temp >> 1;
        }
    }

    return(CRC_Temp);
}



/************
���������ȡʪ��
*************/
u16 Get_Humidity(u8 *buf)
{
    u16 humidity = 0;
    humidity = (buf[5] * 256 + buf[6]) / 100;
    return humidity;
}

/************
���������ȡ�¶�����
*************/
u16 Get_Temperature1(u8 *buf)
{
    u16 temperature1 = 0, temp = 0;
    temp = buf[3] & 0x0f;
    temperature1 = (temp * 256 + buf[4]) / 100;
    return temperature1;
}

/************
���������ȡ�¶�����
*************/
u16 Get_Temperature2(u8 *buf)
{
    u16 temperature2 = 0, temp = 0;
    temp = buf[3] & 0x0f;
    temp = (temp * 256 + buf[4]) % 100;
    temperature2 = temp / 10;
    return temperature2;
}


/************
���������ȡ����ѹ
*************/
u32 Get_Atmosphere(u8 *buf)
{
    u32 atmosphere = 0;
    atmosphere = (((buf[7]) >> 4) * 4096) + ((buf[7] & 0x0f) * 256) + (((buf[8]) >> 4) * 16) + (buf[8] & 0x0f);
    atmosphere = atmosphere * 10;
    return atmosphere;
}


/************
���������ȡ������
*************/
u32 Get_FireAlarm(u8 *buf)
{
    u32 ppm = 0;
    ppm = (((buf[3]) >> 4) * 4096) + ((buf[3] & 0x0f) * 256) + (((buf[4]) >> 4) * 16) + (buf[4] & 0x0f);
    return ppm;
}



