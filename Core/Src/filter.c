#include "main.h"
#include <stdio.h>
#include "string.h"
#include <stdlib.h>
#include "arm_math.h"
#include "filter.h"
#include "cmsis_os.h"

float globalFrequency = 550.0f;
float globalBW = 400.0f;
float globalpeakGain = 0.0f;

float upperMaxTressHold = 0.3f;
float lowerMaxTressHold = -0.3f;

uint32_t sinusStep = 0;
float sinusAmp = 0;

float32_t left_Samles_In_Float_Lower[SIGNAL_SAMPLES/2];
float32_t left_Samles_In_Float_Upper[SIGNAL_SAMPLES/2];

float32_t right_Samles_In_Float_Lower[SIGNAL_SAMPLES/2];
float32_t right_Samles_In_Float_Upper[SIGNAL_SAMPLES/2];

float32_t left_Samles_Out_Float_Lower[SIGNAL_SAMPLES/2];
float32_t left_Samles_Out_Float_Upper[SIGNAL_SAMPLES/2];

float32_t right_Samles_Out_Float_Lower[SIGNAL_SAMPLES/2];
float32_t right_Samles_Out_Float_Upper[SIGNAL_SAMPLES/2];


extern SAI_HandleTypeDef haudio_in_sai;          // forwarded from stm32746g_discovery.c

// IIR filter design tools sets a0 to 1 and CMSIS library just skips it, because it is always 1
// Also depends on the IIR tool used, but a1 and a2 must be set to negative according to the CMSIS library (not multiplied by -1 (reversed), just negative value)
// Anyway check carefully outputs from your design tool also the order of coefficients, values are as follow: {b0,b1,b2,a1,a2}


void filterinit (void)
{

	  /* Initialize Audio RecINPUT_DEVICE_INPUT_LINE order INPUT_DEVICE_INPUT_LINE_1 */
	  if (BSP_AUDIO_IN_OUT_Init(INPUT_DEVICE_INPUT_LINE_1, OUTPUT_DEVICE_HEADPHONE, DEFAULT_AUDIO_IN_FREQ, DEFAULT_AUDIO_IN_BIT_RESOLUTION, DEFAULT_AUDIO_IN_CHANNEL_NBR) == AUDIO_OK)
	  {
	   BSP_AUDIO_IN_SetVolume(45);
	  }
	  wm8994_SetOutputMode(AUDIO_I2C_ADDRESS,OUTPUT_DEVICE_HEADPHONE);

	  /* Initialize SDRAM buffers */
	  memset((uint16_t*)AUDIO_BUFFER_IN, 0, AUDIO_BLOCK_SIZE*2);
	  memset((uint16_t*)AUDIO_BUFFER_OUT, 0, AUDIO_BLOCK_SIZE*2);
	  audio_rec_buffer_state = BUFFER_OFFSET_NONE;

	  /* Start Recording */
	  // Number of elements not size,
	  BSP_AUDIO_IN_Record((uint16_t*)AUDIO_BUFFER_IN, AUDIO_BLOCK_SIZE);

	  /* Start Playback */
	  // Data in bytes, left and right elements
	  BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
	  BSP_AUDIO_OUT_Play((uint16_t*)AUDIO_BUFFER_OUT, AUDIO_BLOCK_SIZE * 2);
	  BSP_AUDIO_IN_SetVolume(45);
	  BSP_AUDIO_OUT_SetVolume(50);

}


void filter (int ftype)
{
  while (1)
  {

	/* Wait end of half block recording */
    while(audio_rec_buffer_state != BUFFER_OFFSET_HALF)
    {
    BSP_PB_GetState(BUTTON_KEY);
    }
    audio_rec_buffer_state = BUFFER_OFFSET_NONE;
   	memcpy((uint16_t *)(AUDIO_BUFFER_OUT),(uint16_t *)(AUDIO_BUFFER_IN),AUDIO_BLOCK_SIZE);

    /* Wait end of one block recording */
    while(audio_rec_buffer_state != BUFFER_OFFSET_FULL)
    {
    	BSP_PB_GetState(BUTTON_KEY);
    }
    audio_rec_buffer_state = BUFFER_OFFSET_NONE;
    /* Copy recorded 2nd half block */
  	memcpy((uint16_t *)((AUDIO_BUFFER_OUT) + (AUDIO_BLOCK_SIZE)),(uint16_t *)((AUDIO_BUFFER_IN) + (AUDIO_BLOCK_SIZE)),AUDIO_BLOCK_SIZE);

  }
}


void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
  audio_rec_buffer_state = BUFFER_OFFSET_FULL;
  return;
}


void BSP_AUDIO_IN_HalfTransfer_CallBack(void)
{
  audio_rec_buffer_state = BUFFER_OFFSET_HALF;
  return;
}


void ExtractSamples_LowerHalf()
{
	q15_t tmp_L = 0;
	q15_t tmp_R = 0;
	uint32_t tmp32 = 0;
	/* Read 16 bit data from the SDRAM memory and convert to float*/
	for (int i = 0; i < SIGNAL_SAMPLES/2; i++)
	{
		tmp32 = *(__IO uint32_t*) (AUDIO_BUFFER_IN + (4*i));
		tmp_L = tmp32;
		left_Samles_In_Float_Lower[i] = tmp_L / 32768.0f;
		tmp32 = tmp32 >> 16;
		tmp_R = tmp32;
		right_Samles_In_Float_Lower[i] = tmp_R / 32768.0f;
	}
}

void ExtractSamples_UpperHalf()
{
	q15_t tmp_L = 0;
	q15_t tmp_R = 0;
	uint32_t tmp32 = 0;
	/* Read 16 bit data from the SDRAM memory and convert to float*/
	for (int i = SIGNAL_SAMPLES/2; i < SIGNAL_SAMPLES; i++)
	{
		tmp32 = *(__IO uint32_t*) (AUDIO_BUFFER_IN + (4*i));
		tmp_L = tmp32;
		left_Samles_In_Float_Upper[i-(SIGNAL_SAMPLES/2)] = tmp_L / 32768.0f;
		tmp32 = tmp32 >> 16;
		tmp_R = tmp32;
		right_Samles_In_Float_Upper[i-(SIGNAL_SAMPLES/2)] = tmp_R / 32768.0f;
	}
}
void InsertSamples_LowerHalf()
{
	uint32_t tmp_L = 0;
	uint32_t tmp_R = 0;
	uint32_t tmp32 = 0;
	/* Write 16 bit data to the SDRAM memory and convert to Q15*/
	for (int i = 0; i < SIGNAL_SAMPLES/2; i++)
	{
    	if (1) {
    		if(sinusStep > 150){sinusStep = 0;}
			sinusAmp = 0.2f;
			float sinusTone = 0.0f;
			sinusTone = sinf(M_2_PI*550.0f*(1.0/1600.0)*sinusStep);
			sinusTone = sinusTone * sinusAmp;
		    arm_float_to_q15(&sinusTone, &tmp_L, 1);
	        arm_float_to_q15(&sinusTone, &tmp_R, 1);
	        sinusStep++;
		} else {
			sinusStep = 0;
		    arm_float_to_q15(&left_Samles_Out_Float_Lower[i], &tmp_L, 1);
	        arm_float_to_q15(&right_Samles_Out_Float_Lower[i], &tmp_R, 1);
		}
		tmp32= (tmp_L <<16) + tmp_R;
		*(__IO uint32_t*) (AUDIO_BUFFER_OUT + (4*i)) = tmp32;
	}
}
void InsertSamples_UpperHalf()
{
	uint32_t tmp_L = 0;
	uint32_t tmp_R = 0;
	uint32_t tmp32 = 0;
	/* Write 16 bit data to the SDRAM memory and convert to Q15*/
	for (int i = SIGNAL_SAMPLES/2; i < SIGNAL_SAMPLES; i++)
	{
		if (1) {
			if(sinusStep > 150){sinusStep = 0;}
			sinusAmp = 0.2f;
			float sinusTone = 0.0f;
			sinusTone = sinf(M_2_PI*550.0f*(1.0/1600.0)*sinusStep);
			sinusTone = sinusTone * sinusAmp;
		    arm_float_to_q15(&sinusTone, &tmp_L, 1);
		    arm_float_to_q15(&sinusTone, &tmp_R, 1);
		    sinusStep++;
		} else {
			sinusStep = 0;
		    arm_float_to_q15(&left_Samles_Out_Float_Upper[i-(SIGNAL_SAMPLES/2)], &tmp_L, 1);
		    arm_float_to_q15(&right_Samles_Out_Float_Upper[i-(SIGNAL_SAMPLES/2)], &tmp_R,1);
		}
		tmp32= (tmp_L <<16) + tmp_R;
		*(__IO uint32_t*) (AUDIO_BUFFER_OUT + (4*i)) = tmp32;
	}
}




