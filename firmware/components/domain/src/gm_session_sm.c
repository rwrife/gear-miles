#include "gm_session_sm.h"

const char *sm_state_name(sm_state_t s)
{
    switch (s) {
    case ST_IDLE: return "idle";
    case ST_RUNNING: return "running";
    case ST_PAUSED: return "paused";
    case ST_FINISHED: return "finished";
    case ST_SETTINGS: return "settings";
    case ST_FRC: return "factory_reset_confirm";
    }
    return "?";
}

uint32_t sm_handle(sm_t *sm, sm_event_t ev, uint32_t now_ms)
{
    /* Transition table mirrors architecture §5 row for row. */
    sm_state_t s = sm->state;
    sm_state_t next = s;
    uint32_t act = 0;

    switch (s) {
    case ST_IDLE:
        if (ev == EV_TAP_A)        { next = ST_RUNNING; act |= SM_ACT_START_SESSION; }
        else if (ev == EV_LONG_B)  { next = ST_SETTINGS; act |= SM_ACT_ENTER_SETTINGS; }
        else if (ev == EV_LONG_AB) { next = ST_FRC; }
        break;
    case ST_FRC:
        if (ev == EV_TAP_A) { next = ST_IDLE; act |= SM_ACT_WIPE; }
        else if (ev == EV_TAP_B) { next = ST_IDLE; act |= SM_ACT_ABORT_WIPE; }
        break;
    case ST_RUNNING:
        if (ev == EV_TAP_A)        { next = ST_FINISHED; }
        else if (ev == EV_TAP_B)   { next = ST_PAUSED; act |= SM_ACT_STOP_TIMER; }
        else if (ev == EV_DROPOUT_5MIN) { next = ST_PAUSED; act |= SM_ACT_STOP_TIMER; }
        break;
    case ST_PAUSED:
        if (ev == EV_TAP_A)      { next = ST_RUNNING; act |= SM_ACT_RESUME_TIMER; }
        else if (ev == EV_TAP_B) { next = ST_FINISHED; }
        break;
    case ST_FINISHED:
        if (ev == EV_FINISHED_TIMEOUT) { next = ST_IDLE; act |= SM_ACT_COMMIT_SESSION; }
        break;
    case ST_SETTINGS:
        if (ev == EV_TAP_B)        { act |= SM_ACT_NEXT_PAGE; }
        else if (ev == EV_TAP_A)   { act |= SM_ACT_ADJUST_VALUE; }
        else if (ev == EV_LONG_B || ev == EV_SETTINGS_TIMEOUT) {
            next = ST_IDLE; act |= SM_ACT_PERSIST_CONFIG;
        }
        break;
    }

    if (next != s) {
        sm->state = next;
        sm->state_since_ms = now_ms;
    }
    return act;
}
