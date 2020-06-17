#include <stdio.h>
#include "kpu.h"
#include <platform.h>
#include <printf.h>
#include <string.h>
#include <stdlib.h>
#include <sysctl.h>
#include <float.h>
#include "uarths.h"
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#define INCBIN_PREFIX
#include "incbin.h"

#define PLL0_OUTPUT_FREQ 1000000000UL
#define PLL1_OUTPUT_FREQ 400000000UL
#define PLL2_OUTPUT_FREQ 45158400UL

INCBIN(model, "hello_kpu.kmodel");

volatile uint32_t g_ai_done_flag;
kpu_model_context_t task1;

static void ai_done(void* userdata)
{
    g_ai_done_flag = 1;
}

int main(void)
{
    /* Set CPU and dvp clk */
    //sysctl_pll_set_freq(SYSCTL_PLL0, PLL0_OUTPUT_FREQ);
    sysctl_pll_set_freq(SYSCTL_PLL1, PLL1_OUTPUT_FREQ);
    //sysctl_pll_set_freq(SYSCTL_PLL2, PLL2_OUTPUT_FREQ);
    sysctl_clock_enable(SYSCTL_CLOCK_AI);
    uarths_init();
    plic_init();
    sysctl_enable_irq();

    int dummy;
    scanf("%d", &dummy);

    printf("system starting ...\n");

#if 0
    for(int i = 0; i < model_size; i++)
    {
        if(i % 8 == 0) {
            printf("%07x    ", i);
        }

        printf("%02x ", model_data[i]);
        if(i % 8 == 7 || i == model_size -1 ) {
            printf("\n");
        }

    }
#endif

    int err_code = kpu_load_kmodel(&task1, model_data);
    if (err_code != 0)
    {
        printf("Cannot load kmodel. %d\n", err_code);
        while(1);
    }

    g_ai_done_flag = 0;

    const float inputs[] = {5.1,3.8,1.9,0.4};

    for(int i = 0; i < 4; i++)
    {
        float input = inputs[i];

        if (kpu_run_kmodel(&task1, (const uint8_t *)&input, 5, ai_done, NULL) != 0)
        {
            printf("Cannot run kmodel.\n");
            while(1);
        }
        while (!g_ai_done_flag);

        float *output;
        size_t output_size;
        kpu_get_output(&task1, 0, (uint8_t **)&output, &output_size);

        printf("input=%f, ideal=%f, kpu=%f\n", input, 2*input - 1, *output);
    }

}