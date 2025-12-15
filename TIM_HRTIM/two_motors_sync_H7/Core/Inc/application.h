/**
  ******************************************************************************
  * @file           : application.h
  * @author         : Luca Cassi

  ******************************************************************************
*/

#ifndef APPLICATION_H
#define APPLICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
  STATE_STOP = 0,
  STATE_RUN  = 1
} AppState_t;

/* Debug-controlled state.
 * Make it volatile so the debugger can modify it and the firmware will observe changes.
 * This is intentionally global and not static to be visible in the IDE watch window.
 */
extern volatile AppState_t appState;

/* Debug-controlled PWM duties in percent (10..90).
 * These are applied to TIM1/TIM8 CCR registers when in STATE_RUN.
 * Default is 50%.
 */
extern volatile uint8_t dbg_m1_duty_a_pct;
extern volatile uint8_t dbg_m1_duty_b_pct;
extern volatile uint8_t dbg_m1_duty_c_pct;

extern volatile uint8_t dbg_m2_duty_a_pct;
extern volatile uint8_t dbg_m2_duty_b_pct;
extern volatile uint8_t dbg_m2_duty_c_pct;

void App_Run(void);

void MotorTimers_SyncStart(void);
void MotorTimers_Stop(void);

#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_H */
