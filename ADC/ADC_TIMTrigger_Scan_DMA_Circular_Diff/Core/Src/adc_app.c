/**
  ******************************************************************************
  * @file    adc_app.c
  * @author  Luca Cassi
  ******************************************************************************
*/

#include "adc_app.h"
#include <string.h>

static ADC_HandleTypeDef *s_hadc = NULL;
static TIM_HandleTypeDef *s_htim = NULL;

// DMA buffer: 2 frames back-to-back (half/full)
// Oversampling does NOT change the number of output samples, only their quality.
static volatile uint16_t s_adc_dma_buf[ADC_APP_DMA_SAMPLES];

// Latest “frame” copied out of DMA (stable view)
static uint16_t s_latest_frame[ADC_APP_REG_CH_COUNT];
static volatile bool s_frame_ready = false;

// Injected latest value
static volatile int16_t s_inj_latest = 0;
static volatile bool s_inj_ready = false;

void adc_app_init(ADC_HandleTypeDef *hadc, TIM_HandleTypeDef *htim_trgo)
{
  s_hadc = hadc;
  s_htim = htim_trgo;
}

HAL_StatusTypeDef adc_app_start(void)
{
  if ((s_hadc == NULL) || (s_htim == NULL)) return HAL_ERROR;

  // Start timer that generates TRGO at 1 kHz
  if (HAL_TIM_Base_Start(s_htim) != HAL_OK) return HAL_ERROR;

  // Start regular conversions in DMA circular mode
  // Length must be number of samples in the circular buffer.
  return HAL_ADC_Start_DMA(s_hadc, (uint32_t*)s_adc_dma_buf, ADC_APP_DMA_SAMPLES);
}

HAL_StatusTypeDef adc_app_stop(void)
{
  if ((s_hadc == NULL) || (s_htim == NULL)) return HAL_ERROR;

  (void)HAL_TIM_Base_Stop(s_htim);
  return HAL_ADC_Stop_DMA(s_hadc);
}

bool adc_app_frame_available(void)
{
  return s_frame_ready;
}

void adc_app_get_latest_frame(uint16_t out_frame[ADC_APP_REG_CH_COUNT])
{
  // simple “consume flag”
  __disable_irq();
  memcpy(out_frame, s_latest_frame, sizeof(s_latest_frame));
  s_frame_ready = false;
  __enable_irq();
}

HAL_StatusTypeDef adc_app_trigger_injected_it(void)
{
  if (s_hadc == NULL) return HAL_ERROR;
  s_inj_ready = false;
  return HAL_ADCEx_InjectedStart_IT(s_hadc);
}

bool adc_app_injected_available(void)
{
  return s_inj_ready;
}

int16_t adc_app_get_injected_latest(void)
{
  s_inj_ready = false;
  return s_inj_latest;
}

const volatile uint16_t* adc_app_get_dma_buffer(void)
{
  return s_adc_dma_buf;
}

// --- HAL callbacks (override weak functions) ---
//
// Called on HALF transfer complete: first frame is ready
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc != s_hadc) return;

  // Frame 0 is [0..4]
  for (uint32_t i = 0; i < ADC_APP_REG_CH_COUNT; i++) {
    s_latest_frame[i] = (uint16_t)s_adc_dma_buf[i];
  }
  s_frame_ready = true;
}

// Called on FULL transfer complete: second frame is ready
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc != s_hadc) return;

  // Frame 1 is [5..9]
  const uint32_t base = ADC_APP_REG_CH_COUNT;
  for (uint32_t i = 0; i < ADC_APP_REG_CH_COUNT; i++) {
    s_latest_frame[i] = (uint16_t)s_adc_dma_buf[base + i];
  }
  s_frame_ready = true;
}

// Injected conversion complete
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc != s_hadc) return;

  // Read injected data register (rank 1)
  // NOTE: interpretation can be signed for differential channels; keep int16_t here.
  uint32_t raw = HAL_ADCEx_InjectedGetValue(hadc, ADC_INJECTED_RANK_1);
  s_inj_latest = (int16_t)(raw & 0xFFFF);
  s_inj_ready = true;
}
