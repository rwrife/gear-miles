/* Gear Miles — session state machine (pure C, host-testable).
 *
 * Implements the frozen table in docs/architecture.md §5 exactly.
 * Events are pre-debounced button gestures plus derived timeouts.
 */
#ifndef GM_SESSION_SM_H
#define GM_SESSION_SM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { ST_IDLE = 0, ST_RUNNING, ST_PAUSED, ST_FINISHED,
               ST_SETTINGS, ST_FRC } sm_state_t;

typedef enum {
    EV_TAP_A = 0,        /* tapA */
    EV_TAP_B,            /* tapB */
    EV_LONG_B,           /* longB  (>= 3 s BTN_B) */
    EV_LONG_AB,          /* longAB (>= 5 s both) */
    EV_DROPOUT_5MIN,     /* RUNNING + 5 min continuous cadence dropout */
    EV_FINISHED_TIMEOUT, /* FINISHED dwell of 2 s elapsed */
    EV_SETTINGS_TIMEOUT, /* SETTINGS idle 60 s */
    EV_COUNT
} sm_event_t;

/* Side effects the caller must perform. */
enum {
    SM_ACT_START_SESSION  = 1u << 0,
    SM_ACT_STOP_TIMER     = 1u << 1, /* RUNNING -> PAUSED */
    SM_ACT_RESUME_TIMER   = 1u << 2,
    SM_ACT_COMMIT_SESSION = 1u << 3, /* FINISHED -> IDLE: write CRC'd record */
    SM_ACT_ENTER_SETTINGS = 1u << 4,
    SM_ACT_PERSIST_CONFIG = 1u << 5, /* settings exit: atomic persist */
    SM_ACT_NEXT_PAGE      = 1u << 6,
    SM_ACT_ADJUST_VALUE   = 1u << 7,
    SM_ACT_WIPE           = 1u << 8, /* confirm: wipe NVS + ring, reboot */
    SM_ACT_ABORT_WIPE     = 1u << 9,
};

typedef struct {
    sm_state_t state;
    uint32_t state_since_ms;
} sm_t;

/* Apply event; returns bitmask of actions, state already advanced. */
uint32_t sm_handle(sm_t *sm, sm_event_t ev, uint32_t now_ms);

const char *sm_state_name(sm_state_t s);

#ifdef __cplusplus
}
#endif
#endif /* GM_SESSION_SM_H */
