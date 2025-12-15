/**
  ******************************************************************************
  * @file           : application.c
  * @author         : Luca Cassi

  ******************************************************************************
*/
  
#include "application.h"
#include "main.h"
#include "tim.h"

/* --- Debug-visible globals --- */
volatile AppState_t appState = STATE_STOP;

/* Default duties: 50% */
volatile uint8_t dbg_m1_duty_a_pct = 50;
volatile uint8_t dbg_m1_duty_b_pct = 50;
volatile uint8_t dbg_m1_duty_c_pct = 50;

volatile uint8_t dbg_m2_duty_a_pct = 50;
volatile uint8_t dbg_m2_duty_b_pct = 50;
volatile uint8_t dbg_m2_duty_c_pct = 50;

/* Local helper: clamp percent into [10..90] */
static uint8_t clamp_pct_10_90(uint8_t pct)
{
  if (pct < 10U) return 10U;
  if (pct > 90U) return 90U;
  return pct;
}

/* Convert duty percent to CCR value based on ARR.
 * Critical: CCR range is 0..ARR (inclusive behavior depends on mode).
 * For PWM1, CCR=ARR/2 yields ~50%.
 */
static uint32_t pct_to_ccr(TIM_HandleTypeDef *htim, uint8_t pct)
{
  uint32_t arr = __HAL_TIM_GET_AUTORELOAD(htim); /* ARR as configured */
  uint32_t p = (uint32_t)clamp_pct_10_90(pct);
  /* Use integer math with rounding */
  return (arr * p) / 100U;
}

/* Apply debug duties to CCR registers.
 * Critical: if CCR preload is enabled, writing CCR updates shadow register and takes effect on next update event (safe).
 * If preload is disabled, duty can change mid-period (may cause jitter/glitches).
 */
static void Apply_DebugDuties(void)
{
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pct_to_ccr(&htim1, dbg_m1_duty_a_pct));
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pct_to_ccr(&htim1, dbg_m1_duty_b_pct));
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, pct_to_ccr(&htim1, dbg_m1_duty_c_pct));

  __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, pct_to_ccr(&htim8, dbg_m2_duty_a_pct));
  __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, pct_to_ccr(&htim8, dbg_m2_duty_b_pct));
  __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, pct_to_ccr(&htim8, dbg_m2_duty_c_pct));
}

void App_Run(void)
{
  static AppState_t prevState = STATE_STOP;

  /* Debug usage:
   * - Change appState in the IDE watch window to STATE_RUN / STATE_STOP
   * - Change dbg_m*_duty_*_pct to values in [10..90]
   * The firmware will apply them continuously while in STATE_RUN.
   *
   * Critical: If later you implement a real FOC loop, it will overwrite CCR values.
   * Keep this debug-only path gated or disable it when control loop is active.
   */

  if (appState != prevState)
  {
    if (appState == STATE_RUN)
    {
      MotorTimers_SyncStart();
    }
    else
    {
      MotorTimers_Stop();
    }
    prevState = appState;
  }

  if (appState == STATE_RUN)
  {
    Apply_DebugDuties();
  }
}

void MotorTimers_SyncStart(void)
{
  /* Stop everything before re-sync */
  (void)HAL_TIM_Base_Stop(&htim2);

  (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
  (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
  (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);

  (void)HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_1);
  (void)HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_2);
  (void)HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_3);

  /* Reset counters for reproducible alignment */
  __HAL_TIM_SET_COUNTER(&htim1, 0U);
  __HAL_TIM_SET_COUNTER(&htim8, 0U);
  __HAL_TIM_SET_COUNTER(&htim2, 0U);

  /* Ensure initial duties are applied before starting PWM outputs */
  Apply_DebugDuties();

  /* Start slaves first (they wait for TRGO) */
  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) { Error_Handler(); }
  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK) { Error_Handler(); }
  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3) != HAL_OK) { Error_Handler(); }

  if (HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1) != HAL_OK) { Error_Handler(); }
  if (HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2) != HAL_OK) { Error_Handler(); }
  if (HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3) != HAL_OK) { Error_Handler(); }

  /* Start master: first update generates TRGO */
  if (HAL_TIM_Base_Start(&htim2) != HAL_OK) { Error_Handler(); }
}

void MotorTimers_Stop(void)
{
  (void)HAL_TIM_Base_Stop(&htim2);

  (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
  (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
  (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);

  (void)HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_1);
  (void)HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_2);
  (void)HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_3);
}
