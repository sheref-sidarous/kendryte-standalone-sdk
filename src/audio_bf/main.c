#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <printf.h>
#include <dmac.h>
#include <plic.h>
#include <i2s.h>
#include <sysctl.h>
#include <dmac.h>
#include <fpioa.h>
#include "apu.h"

#define APU_DIR_DMA_CHANNEL DMAC_CHANNEL3
#define APU_VOC_DMA_CHANNEL DMAC_CHANNEL4

#define APU_DIR_CHANNEL_MAX 16
#define APU_DIR_CHANNEL_SIZE 512
#define APU_VOC_CHANNEL_SIZE 512

#define APU_FFT_ENABLE 0

int count;
int assert_state;

#if APU_FFT_ENABLE
volatile uint32_t APU_DIR_FFT_BUFFER[APU_DIR_CHANNEL_MAX]
				       [APU_DIR_CHANNEL_SIZE]
	__attribute__((aligned(128)));
volatile uint32_t APU_VOC_FFT_BUFFER[APU_VOC_CHANNEL_SIZE]
	__attribute__((aligned(128)));
#else
volatile int16_t APU_DIR_BUFFER[APU_DIR_CHANNEL_MAX]
				  [APU_DIR_CHANNEL_SIZE]
	__attribute__((aligned(128)));
volatile int16_t APU_VOC_BUFFER[APU_VOC_CHANNEL_SIZE]
	__attribute__((aligned(128)));
#endif

int16_t record1[512*1024];
int16_t record2[512*1024];
int16_t record3[512*1024];
#define counter_max 16

int dir_logic(void* ctx)
{
	static uint64_t counter = 0;
	if(counter < counter_max){
		for(int i=0; i<512; i++){
			record1[512*counter + i] = APU_DIR_BUFFER[0][i];
			record2[512*counter + i] = APU_DIR_BUFFER[3][i];
			record3[512*counter + i] = APU_DIR_BUFFER[7][i];
		}
	}
	if(counter == counter_max){
		for(int i=0; i<512*counter_max; i++){
			printk("%d\t%d\t%d\n", record1[i], record2[i], record3[i]);
		}
		while(1);
	}
	printk("%s %lu\n", __func__, counter++);

	apu_dir_enable();
	return 0;
}

int voc_logic(void* ctx)
{
	static uint64_t counter = 0;
	// if(counter < 1){
	// 	for(int i=0; i<512; i++){
	// 		record[512*counter + i] = APU_VOC_BUFFER[i];
	// 	}
	// }
	// if(counter == 1){
	// 	for(int i=0; i<512*1; i++){
	// 		printk("%d,\n", record[i]);
	// 	}
	// 	while(1);
	// }
	printk("%s %lu\n", __func__, counter++);
	return 0;
}


#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"

#define NN 128
#define TAIL 1024

int main2(void){
	short echo_buf[NN], ref_buf[NN], e_buf[NN];
	SpeexEchoState *st;
	SpeexPreprocessState *den;
	int sampleRate = 8000;

	st = speex_echo_state_init(NN, TAIL);
	den = speex_preprocess_state_init(NN, sampleRate);
	speex_echo_ctl(st, SPEEX_ECHO_SET_SAMPLING_RATE, &sampleRate);
	speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_ECHO_STATE, st);


		speex_echo_cancellation(st, ref_buf, echo_buf, e_buf);
		speex_preprocess_run(den, e_buf);

   speex_echo_state_destroy(st);
   speex_preprocess_state_destroy(den);

	return 0;
}


int main1(void)
{
	printk("git id: %u\n", sysctl->git_id.git_id);
	printk("init start.\n");
	clear_csr(mie, MIP_MEIP);
	//init_all();
	apu_init_default(
		1, 39, 46, 42, 43, 44, 45,
		1, 0, 1, APU_DIR_DMA_CHANNEL, APU_VOC_DMA_CHANNEL,
		1, 44100, RESOLUTION_16_BIT, SCLK_CYCLES_32, 
		TRIGGER_LEVEL_4, STANDARD_MODE,
		APU_FFT_ENABLE, 1, 0, 
#if APU_FFT_ENABLE
		(void*)APU_DIR_FFT_BUFFER, (void*)APU_VOC_FFT_BUFFER
#else
		(void*)APU_DIR_BUFFER, (void*)APU_VOC_BUFFER
#endif
	);
	printk("init done.\n");
//	apu_print_setting();
	set_csr(mie, MIP_MEIP);
	set_csr(mstatus, MSTATUS_MIE);

	while(event_loop_step(dir_logic, voc_logic) | 1){}
}