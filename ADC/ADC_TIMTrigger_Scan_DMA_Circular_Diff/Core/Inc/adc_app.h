/**
  ******************************************************************************
  * @file    adc_app.h
  * @author  Luca Cassi
  ******************************************************************************
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// Regular scan configuration (must match CubeMX ranks)
#define ADC_APP_REG_CH_COUNT     5u
#define ADC_APP_DMA_FRAMES       2u  // ping-pong via half/full callbacks
#define ADC_APP_DMA_SAMPLES      (ADC_APP_REG_CH_COUNT * ADC_APP_DMA_FRAMES)

// Index mapping in each frame (order == ranks)
typedef enum {
  ADC_APP_CH_IN2_DIFF = 0,
  ADC_APP_CH_IN3_SE   = 1,
  ADC_APP_CH_IN4_SE   = 2,
  ADC_APP_CH_IN8_SE   = 3,
  ADC_APP_CH_IN9_SE   = 4
} adc_app_ch_t;

// Public API
void adc_app_init(ADC_HandleTypeDef *hadc, TIM_HandleTypeDef *htim_trgo);
HAL_StatusTypeDef adc_app_start(void);
HAL_StatusTypeDef adc_app_stop(void);

// Regular frames
bool adc_app_frame_available(void);
void adc_app_get_latest_frame(uint16_t out_frame[ADC_APP_REG_CH_COUNT]);

// Injected (software start) - result read via callback
HAL_StatusTypeDef adc_app_trigger_injected_it(void);
bool adc_app_injected_available(void);
int16_t adc_app_get_injected_latest(void);

// Optional: expose raw DMA buffer for debug
const volatile uint16_t* adc_app_get_dma_buffer(void);

#ifdef __cplusplus
}
#endif
