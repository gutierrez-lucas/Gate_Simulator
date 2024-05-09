#ifndef ___GATE_SIMULATOR_H
#define ___GATE_SIMULATOR_H

#define TIME_UPPER_LIMIT 4000
#define TIME_LOWER_LIMIT 0

typedef struct gs_s gs_s;
typedef enum gs_status_e{
    GS_OPENED,
    GS_CLOSED,
    GS_MOVING,
    GS_STOPPED
}gs_status_e;

gs_s* gate_simulator_init(gs_status_e initial_status);
void gate_simulator_run(gs_s* new_gate);
void gate_simulator_end();

#endif