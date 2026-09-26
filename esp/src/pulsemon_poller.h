#ifndef PULSEMON_POLLER_H
#define PULSEMON_POLLER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PULSEMON_BACKEND_UNKNOWN = 0,
    PULSEMON_BACKEND_ONLINE,
    PULSEMON_BACKEND_SUSPECT,
    PULSEMON_BACKEND_OFFLINE,
} pulsemon_backend_state_t;

void pulsemon_poller_start(void);
pulsemon_backend_state_t pulsemon_poller_get_backend_state(void);
bool pulsemon_poller_wait_initial_state(uint32_t timeout_ms);
void pulsemon_poller_set_ui_ready(bool ready);

#endif
