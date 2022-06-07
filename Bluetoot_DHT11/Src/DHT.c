
#include "DHT.h"
//************************** Low Level Layer ********************************************************//
#include "delay_timer.h"
#include "math.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#define Hex 0x30
static void DHT_DelayInit(DHT_Name* DHT)
{
	DELAY_TIM_Init(DHT->Timer);
}
static void DHT_DelayUs(DHT_Name* DHT, uint16_t Time)
{
	DELAY_TIM_Us(DHT->Timer, Time);
}

static void DHT_SetPinOut(DHT_Name* DHT)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = DHT->Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT->PORT, &GPIO_InitStruct);
}
static void DHT_SetPinIn(DHT_Name* DHT)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = DHT->Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT->PORT, &GPIO_InitStruct);
}
static void DHT_WritePin(DHT_Name* DHT, uint8_t Value)
{
	HAL_GPIO_WritePin(DHT->PORT, DHT->Pin, Value);
}
static uint8_t DHT_ReadPin(DHT_Name* DHT)
{
	uint8_t Value;
	Value =  HAL_GPIO_ReadPin(DHT->PORT, DHT->Pin);
	return Value;
}
//********************************* Middle level Layer ****************************************************//
static uint8_t DHT_Start(DHT_Name* DHT)
{
	uint8_t Response = 0;
	DHT_SetPinOut(DHT);  
	DHT_WritePin(DHT, 0);
	DHT_DelayUs(DHT, DHT->Type);
	
	DHT_WritePin(DHT, 1);
	DHT_DelayUs(DHT, 40);
	DHT_SetPinIn(DHT);
	
	if (!DHT_ReadPin(DHT))
	{
		DHT_DelayUs(DHT, 80); 
		if(DHT_ReadPin(DHT))
		{
			Response = 1;   
		}
		else Response = 0;  
	}		
	while(DHT_ReadPin(DHT));

	return Response;
}
static uint8_t DHT_Read(DHT_Name* DHT)
{
	uint8_t Value = 0;
	DHT_SetPinIn(DHT);
	for(int i = 0; i<8; i++)
	{
		while(!DHT_ReadPin(DHT));
		DHT_DelayUs(DHT, 40);
		if(!DHT_ReadPin(DHT))
		{
			Value &= ~(1<<(7-i));	
		}
		else Value |= 1<<(7-i);
		while(DHT_ReadPin(DHT));
	}
	return Value;
}

//************************** High Level Layer ********************************************************//
void DHT_Init(DHT_Name* DHT, uint8_t DHT_Type, TIM_HandleTypeDef* Timer, GPIO_TypeDef* DH_PORT, uint16_t DH_Pin)
{
	if(DHT_Type == DHT11)
	{
		DHT->Type = DHT11_STARTTIME;
	}
	else if(DHT_Type == DHT22)
	{
		DHT->Type = DHT22_STARTTIME;
	}
	DHT->PORT = DH_PORT;
	DHT->Pin = DH_Pin;
	DHT->Timer = Timer;
	DHT_DelayInit(DHT);
}
float Decimal(uint8_t a, int dem)
{
	int b=a;
	while ((b/10) > 1)
	{
		b = b/10;
		dem ++;
	}
	return dem;
}
uint8_t DHT_ReadTempHum(DHT_Name* DHT)
{
	uint8_t Temp1, Temp2, RH1, RH2;
	uint16_t SUM = 0;
	DHT_Start(DHT);
	RH1 = DHT_Read(DHT);
	RH2 = DHT_Read(DHT);
	Temp1 = DHT_Read(DHT);
	Temp2 = DHT_Read(DHT);
	SUM = DHT_Read(DHT);
	if (SUM == (Temp1 + Temp2 + RH1 + RH2))
	{
		DHT->Temp10 = (Temp1*pow(10,Decimal(Temp2,1)) + Temp2);
		DHT->Humi10 = (RH1*pow(10,Decimal(RH2,1)) + RH2);
		
		DHT->Temp[0]= (DHT->Temp10)/100 + Hex;
		DHT->Temp[2]= 0x2E;
		DHT->Temp[3]= (DHT->Temp10)%10 + Hex;
		DHT->Temp[1]= ((DHT->Temp10)/10)%10 + Hex;
		
		DHT->Humi[0]= (DHT->Humi10)/100 + Hex;
		DHT->Humi[2]= 0x2E;
		DHT->Humi[3]= (DHT->Humi10)%10 + Hex;
		DHT->Humi[1]= ((DHT->Humi10)/10)%10 + Hex;
		
		DHT->Humi[4]= 37;
		DHT->Humi[5]= 0x0A;
		DHT->Temp[4]= 176;
		DHT->Temp[5]= 67;
		DHT->Temp[6]= 0x0A;
	}		
	
	return SUM;
}
