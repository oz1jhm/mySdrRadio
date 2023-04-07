/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FILTER_H
#define __FILTER_H

#include "arm_math.h"

#ifdef __cplusplus
 extern "C" {
#endif

 typedef enum
 {
   BUFFER_OFFSET_NONE = 0,
   BUFFER_OFFSET_HALF = 1,
   BUFFER_OFFSET_FULL = 2,
 }BUFFER_StateTypeDef;

 typedef enum
 {
   lowPassFilter = 0,
   highPassFilter = 1,
   bandPassFilter = 2,
   peakFilter = 3,
 }Filter_Type_Def;

 uint32_t  audio_rec_buffer_state;

 #define VOLIIR 80

#define AUDIO_REC_START_ADDR         ((uint32_t)0xC00FF000)

 #define AUDIO_BLOCK_SIZE   ((uint32_t)256)
 #define SIGNAL_SAMPLES		AUDIO_BLOCK_SIZE/2
 #define AUDIO_BUFFER_IN    AUDIO_REC_START_ADDR     /* In SDRAM */
 #define AUDIO_BUFFER_OUT   (AUDIO_REC_START_ADDR + (AUDIO_BLOCK_SIZE * 2)) /* In SDRAM */
 #define float_buffer_in    (AUDIO_REC_START_ADDR + (AUDIO_BLOCK_SIZE * 4))
 #define float_buffer_out   (AUDIO_REC_START_ADDR + (AUDIO_BLOCK_SIZE * 6))

 #define FFT_SIZE    SIGNAL_SAMPLES
 arm_rfft_fast_instance_f32 fft_inst;
 float32_t fft_in[FFT_SIZE], fft_out[FFT_SIZE];
 float32_t fft_mag[FFT_SIZE>>1];

 int numStages_IIR = 1 ;
 int NUM_TAPS_IIR  = 5;
 float32_t pStateIIR_L[100];
 float32_t pStateIIR_R[100];

 arm_biquad_cascade_df2T_instance_f32 S_L;
 arm_biquad_cascade_df2T_instance_f32 S_R;

// unsigned char logmsgbuff[128];
 extern uint32_t  audio_rec_buffer_state;

 float iir_coeffs[100];

 void CalcFilters(Filter_Type_Def filter, float Frequency, float Q, float peakGain, int stageNumber);
 void filterinit (void);
 void filterCoffInint (int ftype,float Frequency, float BW, float peakGain);
 void filter (int ftype);
 void ExtractSamples_LowerHalf();
 void ExtractSamples_UpperHalf();
 void InsertSamples_LowerHalf();
 void InsertSamples_UpperHalf();


 float globalFrequency;
 float globalBW;
 float globalpeakGain;

#ifdef __cplusplus
}
#endif

#endif /* __FILTER_H */


