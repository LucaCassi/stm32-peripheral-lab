/**
  ******************************************************************************
  * @file           : application.c
  * @author         : Luca Cassi
  ******************************************************************************
*/

#include "application.h"
#include "main.h"
#include "hrtim.h"
#include "stm32g4xx_hal_hrtim.h"          // added just to avoid a warning during compilation

/* From .ioc:
 * HRTIM1Freq = 170 MHz
 * Period for all timers A..F = 17000 ticks
 * Initial CMP1 = 8500 ticks (50%)
 */
#define HRTIM_PWM_PERIOD_TICKS   (17000U)

/* Debug globals */
volatile AppState_t appState = STATE_STOP;

volatile uint8_t dbg_m1_u_pct = 50;
volatile uint8_t dbg_m1_v_pct = 50;
volatile uint8_t dbg_m1_w_pct = 50;

volatile uint8_t dbg_m2_u_pct = 50;
volatile uint8_t dbg_m2_v_pct = 50;
volatile uint8_t dbg_m2_w_pct = 50;

volatile bool dbg_outputs_enable = true;

/* Local shadow copy to avoid rewriting CMP registers every loop iteration */
static uint8_t s_m1_u_last = 0xFF;
static uint8_t s_m1_v_last = 0xFF;
static uint8_t s_m1_w_last = 0xFF;
static uint8_t s_m2_u_last = 0xFF;
static uint8_t s_m2_v_last = 0xFF;
static uint8_t s_m2_w_last = 0xFF;

static uint8_t clamp_pct_10_90(uint8_t pct)
{
  if (pct < 10U) return 10U;
  if (pct > 90U) return 90U;
  return pct;
}

/* Convert duty percent into CMP ticks.
 * Critical:
 * - CMP=0 or CMP=PERIOD can create corner cases (always ON/OFF).
 * - We clamp to [10..90]% to avoid problematic extremes during bring-up.
 */
static uint32_t pct_to_cmp_ticks(uint8_t pct)
{
  uint32_t p = (uint32_t)clamp_pct_10_90(pct);
  return (HRTIM_PWM_PERIOD_TICKS * p) / 100U;
}

/* Uses only HAL APIs available in stm32g4xx_hal_hrtim.c (Waveform mode).
 * Requires:
 *   #include "hrtim.h"
 *   (and HAL_HRTIM_MODULE_ENABLED in stm32g4xx_hal_conf.h)
 *
 * Notes:
 * - This updates CMP1 for timers A..F.
 * - With preload enabled in CubeMX, the new compare value is applied at the next update event.
 */
static void Apply_DutyIfChanged(void)
{
  uint8_t m1u = clamp_pct_10_90(dbg_m1_u_pct);
  uint8_t m1v = clamp_pct_10_90(dbg_m1_v_pct);
  uint8_t m1w = clamp_pct_10_90(dbg_m1_w_pct);
  uint8_t m2u = clamp_pct_10_90(dbg_m2_u_pct);
  uint8_t m2v = clamp_pct_10_90(dbg_m2_v_pct);
  uint8_t m2w = clamp_pct_10_90(dbg_m2_w_pct);

  /* Write back clamped values so the debugger always shows safe bounds. */
  dbg_m1_u_pct = m1u; dbg_m1_v_pct = m1v; dbg_m1_w_pct = m1w;
  dbg_m2_u_pct = m2u; dbg_m2_v_pct = m2v; dbg_m2_w_pct = m2w;

  /* Common compare configuration: no auto-delayed mode for CMP1. */
  HRTIM_CompareCfgTypeDef cmpCfg;
  cmpCfg.AutoDelayedMode    = HRTIM_AUTODELAYEDMODE_REGULAR;
  cmpCfg.AutoDelayedTimeout = 0;

  if (m1u != s_m1_u_last)
  {
    cmpCfg.CompareValue = pct_to_cmp_ticks(m1u);
    if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1, &cmpCfg) != HAL_OK)
      Error_Handler();
    s_m1_u_last = m1u;
  }

  if (m1v != s_m1_v_last)
  {
    cmpCfg.CompareValue = pct_to_cmp_ticks(m1v);
    if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_1, &cmpCfg) != HAL_OK)
      Error_Handler();
    s_m1_v_last = m1v;
  }

  if (m1w != s_m1_w_last)
  {
    cmpCfg.CompareValue = pct_to_cmp_ticks(m1w);
    if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_C, HRTIM_COMPAREUNIT_1, &cmpCfg) != HAL_OK)
      Error_Handler();
    s_m1_w_last = m1w;
  }

  if (m2u != s_m2_u_last)
  {
    cmpCfg.CompareValue = pct_to_cmp_ticks(m2u);
    if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_D, HRTIM_COMPAREUNIT_1, &cmpCfg) != HAL_OK)
      Error_Handler();
    s_m2_u_last = m2u;
  }

  if (m2v != s_m2_v_last)
  {
    cmpCfg.CompareValue = pct_to_cmp_ticks(m2v);
    if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_E, HRTIM_COMPAREUNIT_1, &cmpCfg) != HAL_OK)
      Error_Handler();
    s_m2_v_last = m2v;
  }

  if (m2w != s_m2_w_last)
  {
    cmpCfg.CompareValue = pct_to_cmp_ticks(m2w);
    if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_F, HRTIM_COMPAREUNIT_1, &cmpCfg) != HAL_OK)
      Error_Handler();
    s_m2_w_last = m2w;
  }
}


void App_Run(void)
{
  static AppState_t prev = STATE_STOP;

  /* Debug-driven behavior:
   * - Modify appState (STOP/RUN) and duty % variables live in the IDE.
   * - Duties are clamped to [10..90]% to avoid corner cases during bring-up.
   */

  if (appState != prev)
  {
    if (appState == STATE_RUN)
    {
      MotorHRTIM_SyncStart();
    }
    else
    {
      MotorHRTIM_Stop();
    }
    prev = appState;
  }

  /* Apply compare updates only in RUN to keep STOP fully quiet. */
  if (appState == STATE_RUN)
  {
    Apply_DutyIfChanged();

    if (!dbg_outputs_enable)
    {
      /* Optional: allow counters running with outputs disabled for safe debugging. */
      uint32_t outputs_mask =
          HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 |
          HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2 |
          HRTIM_OUTPUT_TC1 | HRTIM_OUTPUT_TC2 |
          HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2 |
          HRTIM_OUTPUT_TE1 | HRTIM_OUTPUT_TE2 |
          HRTIM_OUTPUT_TF1 | HRTIM_OUTPUT_TF2;
      (void)HAL_HRTIM_WaveformOutputStop(&hhrtim1, outputs_mask);
    }
  }
  else
  {
    /* In STOP, still clamp variables so they remain safe in the debugger. */
    (void)clamp_pct_10_90(dbg_m1_u_pct);
    (void)clamp_pct_10_90(dbg_m1_v_pct);
    (void)clamp_pct_10_90(dbg_m1_w_pct);
    (void)clamp_pct_10_90(dbg_m2_u_pct);
    (void)clamp_pct_10_90(dbg_m2_v_pct);
    (void)clamp_pct_10_90(dbg_m2_w_pct);
  }
}

void MotorHRTIM_SyncStart(void)
{
  MotorHRTIM_Stop();

  /* Ensure compares are set to current debug values before enabling outputs. */
  Apply_DutyIfChanged();

  uint32_t timers_mask =
      HRTIM_TIMERID_TIMER_A |
      HRTIM_TIMERID_TIMER_B |
      HRTIM_TIMERID_TIMER_C |
      HRTIM_TIMERID_TIMER_D |
      HRTIM_TIMERID_TIMER_E |
      HRTIM_TIMERID_TIMER_F;

  if (HAL_HRTIM_WaveformCounterStart(&hhrtim1, timers_mask) != HAL_OK)
  {
    Error_Handler();
  }

  if (dbg_outputs_enable)
  {
    uint32_t outputs_mask =
        HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 |
        HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2 |
        HRTIM_OUTPUT_TC1 | HRTIM_OUTPUT_TC2 |
        HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2 |
        HRTIM_OUTPUT_TE1 | HRTIM_OUTPUT_TE2 |
        HRTIM_OUTPUT_TF1 | HRTIM_OUTPUT_TF2;

    if (HAL_HRTIM_WaveformOutputStart(&hhrtim1, outputs_mask) != HAL_OK)
    {
      Error_Handler();
    }
  }
}

void MotorHRTIM_Stop(void)
{
  uint32_t outputs_mask =
      HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 |
      HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2 |
      HRTIM_OUTPUT_TC1 | HRTIM_OUTPUT_TC2 |
      HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2 |
      HRTIM_OUTPUT_TE1 | HRTIM_OUTPUT_TE2 |
      HRTIM_OUTPUT_TF1 | HRTIM_OUTPUT_TF2;

  (void)HAL_HRTIM_WaveformOutputStop(&hhrtim1, outputs_mask);

  uint32_t timers_mask =
      HRTIM_TIMERID_TIMER_A |
      HRTIM_TIMERID_TIMER_B |
      HRTIM_TIMERID_TIMER_C |
      HRTIM_TIMERID_TIMER_D |
      HRTIM_TIMERID_TIMER_E |
      HRTIM_TIMERID_TIMER_F;

  (void)HAL_HRTIM_WaveformCounterStop(&hhrtim1, timers_mask);


}
