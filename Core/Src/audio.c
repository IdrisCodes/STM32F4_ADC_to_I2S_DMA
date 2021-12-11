
#include "audio.h"
#include "cs43l22.h"
#include "i2s.h"
#include "adc.h"
#include "tim.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>


#define BUFFER_SIZE (2048U)



/* Audio IN buffer */
static volatile uint16_t 	audioInBuffer[2*BUFFER_SIZE];

/* Audio OUT buffer is double the size of the IN buffer
 * because we need to duplicate the samples to have stereo.
 * The CODEC seems not to support MONO. */
static volatile uint16_t 	audioOutBuffer[2*2*BUFFER_SIZE];


/* Conversion time for a buffer */
static volatile uint16_t conv_time_us = 0U;

/* I2S transmission time for a buffer */
static volatile uint16_t i2s_time_us = 0U;

/* I2S transmission time for a buffer */
static volatile uint16_t filter_time_us = 0U;


static volatile uint32_t i2s_half = 0U;
static volatile uint32_t i2s_full = 0U;

static volatile uint32_t adc_half = 0U;
static volatile uint32_t adc_full = 0U;



void Audio_Init(void)
{
	/* Bring audio DAC nRESET pin high */
	HAL_GPIO_WritePin(Audio_RST_GPIO_Port,Audio_RST_Pin,GPIO_PIN_RESET);
	HAL_Delay(10);
	HAL_GPIO_WritePin(Audio_RST_GPIO_Port,Audio_RST_Pin,GPIO_PIN_SET);
	HAL_Delay(10);
	
	/* Initialize DAC*/
	if(cs43l22_Init(AUDIO_I2C_ADDRESS,OUTPUT_DEVICE_HEADPHONE,70,AUDIO_FREQUENCY_44K))
	{
		/* Communication error with CODEC */
		HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
		printf("There's been a problem with the CODEC\r\n");
	}
	else
	{
		cs43l22_Play(AUDIO_I2C_ADDRESS, NULL,0);
	}
}

void Audio_Loop(void)
{
	HAL_TIM_Base_Start(&htim2);
	HAL_I2S_Transmit_DMA(&hi2s3,(uint16_t*)audioOutBuffer,2*2*BUFFER_SIZE);
	while(1)
	{
		/* Wait for half transfer complete */
		while(i2s_half == 0U);
		i2s_half = 0U;
		/* Duplicate samples for left and right channels and store them
		 * in the output buffer */
		for(int i =0 ;i<BUFFER_SIZE; ++i)
		{
			audioOutBuffer[2*i] = (audioInBuffer[i]);
			audioOutBuffer[2*i+1] = (audioInBuffer[i]);
		}


		while(i2s_full == 0U);
		i2s_full = 0U;
		for(int i =BUFFER_SIZE;i<2*BUFFER_SIZE; ++i)
		{
			audioOutBuffer[2*i] = (audioInBuffer[i]);
			audioOutBuffer[2*i+1] = (audioInBuffer[i]);
		}
	}
}


void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
	HAL_GPIO_TogglePin(TEST_PT0_GPIO_Port, TEST_PT0_Pin);
	adc_half = 1U;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	HAL_GPIO_TogglePin(TEST_PT0_GPIO_Port, TEST_PT0_Pin);
	adc_full = 1U;
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
	i2s_half = 1U;
	HAL_GPIO_TogglePin(TEST_PT1_GPIO_Port, TEST_PT1_Pin);
	HAL_ADC_Start_DMA(&hadc3, (uint32_t*)audioInBuffer, BUFFER_SIZE);

}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
	i2s_full = 1U;
	HAL_GPIO_TogglePin(TEST_PT1_GPIO_Port, TEST_PT1_Pin);
	HAL_ADC_Start_DMA(&hadc3, (uint32_t*)(audioInBuffer + BUFFER_SIZE), BUFFER_SIZE);

}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
	printf("ADC Error\n");
}

/* Error callback for I2S and I2C */
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
	printf("I2S Error\r\n");
	HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_SET);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	printf("I2C Error\r\n");
	HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_SET);
}



