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

/* Application states */
typedef enum
{
  STATE_STOP = 0,
  STATE_RUN  = 1
} AppState_t;

/* Debug-controlled state (modifiable from IDE watch window). */
extern volatile AppState_t appState;

/* Debug-controlled PWM duties in percent (clamped to 10..90), default 50%.
 * Motor 1 phases: A/B/C  (U/V/W)
 * Motor 2 phases: D/E/F  (U/V/W)
 */
extern volatile uint8_t dbg_m1_u_pct;
extern volatile uint8_t dbg_m1_v_pct;
extern volatile uint8_t dbg_m1_w_pct;

extern volatile uint8_t dbg_m2_u_pct;
extern volatile uint8_t dbg_m2_v_pct;
extern volatile uint8_t dbg_m2_w_pct;

/* Optional: allow keeping counters running while outputs are disabled for safe bring-up. */
extern volatile bool dbg_outputs_enable;

/* Run one iteration of the application (call from while(1)). */
void App_Run(void);

/* Start/stop of synchronized dual 3-phase PWM using HRTIM timers A..F. */
void MotorHRTIM_SyncStart(void);
void MotorHRTIM_Stop(void);

#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_H */
