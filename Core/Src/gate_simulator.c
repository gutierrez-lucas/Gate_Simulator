#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO

#include "main.h"
#include "tools.h"
#include "gate_simulator.h"

#define DRAW_TOOL_SCREEN_SIZE 100
void draw_door(uint8_t proportion);
uint8_t get_proportion(gs_s* gate_sim);
void print_header(gs_s* gate_sim);

#define turn_direct_light() led_switch(LED_BLUE1, ON); led_switch(LED_BLUE2, OFF); led_switch(LED_BLUE3, OFF); led_switch(LED_GREEN, OFF)
#define turn_reverse_light() led_switch(LED_BLUE1, OFF); led_switch(LED_BLUE2, ON); led_switch(LED_BLUE3, OFF); led_switch(LED_GREEN, OFF)
#define turn_open_light() led_switch(LED_BLUE1, OFF); led_switch(LED_BLUE2, OFF); led_switch(LED_BLUE3, ON); led_switch(LED_GREEN, OFF)
#define turn_close_light() led_switch(LED_BLUE1, OFF); led_switch(LED_BLUE2, OFF); led_switch(LED_BLUE3, OFF); led_switch(LED_GREEN, ON)
#define turn_stop_light() led_switch(LED_BLUE1, OFF); led_switch(LED_BLUE2, OFF) 

extern sw_s *sw1, *sw2;

extern uint16_t get_count();
extern void run_counter();
extern void stop_counter();
extern void reset(uint16_t);
extern void change_direction(bool);

extern UART_HandleTypeDef huart1;

void gate_simulator_run(gs_s* gate_sim);
void gate_simulator_get_event(gs_s* gate_sim);
void gate_simulator_state_machine(gs_s* gate_sim);

typedef enum {
    IDLE,
    OPENING,
    CLOSING,
    STOPPED
}gs_sm_state_e;

typedef enum {
    OPEN_COMMAND,
    CLOSE_COMMAND,
    DUAL_COMMAND,
    STOP_COMMAND,
    OPEN_END,
    CLOSE_END
}gs_sm_event_e;

typedef struct gs_sm_s{
    gs_sm_state_e state;
    gs_sm_event_e event;
}gs_sm_s;

typedef struct count_s{
    uint16_t (*get_count)();
    void (*run)();
    void (*stop)();
    void (*reset)(uint16_t);
    void (*change_direction)(bool)
}count_s;

typedef struct gs_s{ 
    gs_sm_s fsm;
    gs_status_e status;
    count_s counter;
    uint16_t moving_timer; 
}gs_s;

gs_s* gate_simulator_init(gs_status_e initial_status){
    gs_s* new_gate = (gs_s*)malloc(sizeof(gs_s));
    
    printf("Gate simulator initialized\n");
    new_gate -> fsm.state = IDLE;
    new_gate -> fsm.event = STOP_COMMAND;
    new_gate -> status = initial_status;

    new_gate -> counter.get_count = &get_count;
    new_gate -> counter.run = &run_counter;
    new_gate -> counter.stop = &stop_counter;
    new_gate -> counter.reset = &reset;
    new_gate -> counter.change_direction = &change_direction;

    new_gate -> counter.stop();

    if(initial_status == GS_CLOSED){
        new_gate -> counter.reset(4000);
        turn_close_light();
    }else if(initial_status == GS_OPENED){
        new_gate -> counter.reset(0);
        turn_open_light();
    }

	sw1 = sw_init(SW1, "Direct Switch", 0, 0);
	sw2 = sw_init(SW2, "Reverse Switch", 0, 0);

    return new_gate;
}

void gate_simulator_run(gs_s* gate_sim){
    gate_simulator_get_event(gate_sim);
    gate_simulator_state_machine(gate_sim);
}


void gate_simulator_get_event(gs_s* gate_sim){
    tools_run();

    print_header(gate_sim);
    draw_door(get_proportion(gate_sim));
    
    if(get_switch_state(sw1) == SW_PRESSED){
        if(get_switch_state(sw2) == SW_PRESSED){
            turn_stop_light();
            gate_sim -> fsm.event = DUAL_COMMAND;
            gate_sim -> counter.stop();
        }else{
            if(gate_sim -> counter.get_count() == TIME_LOWER_LIMIT){
                // printf("Gate opened\r\n");
                gate_sim -> fsm.event = OPEN_END;
                gate_sim -> counter.stop();
                turn_open_light();
            }else{
                gate_sim -> fsm.event = OPEN_COMMAND;
                gate_sim -> counter.change_direction(0);
                gate_sim -> counter.run();
                turn_direct_light();
            }
        }
    }else if(get_switch_state(sw2) == SW_PRESSED){
        if(get_switch_state(sw1) == SW_PRESSED){
            gate_sim -> fsm.event = DUAL_COMMAND;
            gate_sim -> counter.stop();
        }else{
            if(gate_sim -> counter.get_count() == TIME_UPPER_LIMIT){
                // printf("Gate closed\r\n");
                gate_sim -> fsm.event = CLOSE_END;
                gate_sim -> counter.stop();
                turn_close_light();
            }else{
                gate_sim -> fsm.event = CLOSE_COMMAND;
                gate_sim -> counter.change_direction(1);
                gate_sim -> counter.run();
                turn_reverse_light();
            }
        }
    }else{
        gate_sim -> fsm.event = STOP_COMMAND;
        gate_sim -> counter.stop();
        turn_stop_light();
    }
}

void gate_simulator_state_machine(gs_s* gate_sim){
    switch( gate_sim -> fsm.state ){
        case IDLE:
            switch(gate_sim -> fsm.event){
                case OPEN_COMMAND:
                    gate_sim -> fsm.state = OPENING;
                    gate_sim -> status = GS_MOVING;
                    break;
                case CLOSE_COMMAND:
                    gate_sim -> fsm.state = CLOSING;
                    gate_sim -> status = GS_MOVING;
                    break;
                default: break;
            }
            break;
        
        case OPENING:
            switch(gate_sim -> fsm.event){
                case DUAL_COMMAND:
                    gate_sim -> fsm.state = STOPPED;
                    gate_sim -> status = GS_STOPPED;
                    break;
                case STOP_COMMAND:
                    gate_sim -> fsm.state = IDLE;
                    gate_sim -> status = GS_STOPPED;
                    break;
                case OPEN_END:
                    gate_sim -> fsm.state = IDLE;
                    gate_sim -> status = GS_OPENED;
                    break;
                default: break;
            }
            break;

        case CLOSING:
            switch(gate_sim -> fsm.event){
                case DUAL_COMMAND:
                    gate_sim -> fsm.state = STOPPED;
                    gate_sim -> status = GS_STOPPED;
                    break;
                case STOP_COMMAND:
                    gate_sim -> fsm.state = IDLE;
                    gate_sim -> status = GS_STOPPED;
                case CLOSE_END:
                    gate_sim -> fsm.state = IDLE;
                    gate_sim -> status = GS_CLOSED;
                    break;
                default: break;
            }
            break;

        case STOPPED:
            switch(gate_sim -> fsm.event){
                case OPEN_COMMAND:
                    gate_sim -> fsm.state = OPENING;
                    gate_sim -> status = GS_MOVING;
                    break;
                case CLOSE_COMMAND:
                    gate_sim -> fsm.state = CLOSING;
                    gate_sim -> status = GS_MOVING;
                    break;
                case STOP_COMMAND:
                    gate_sim -> fsm.state = IDLE;
                    gate_sim -> status = GS_STOPPED;
                    break;
                default: break;
            }
            break;
        
        default: break;
    }
}

void gate_simulator_end(gs_s* gate_sim){
    printf("Gate simulator ended\n");
    free(gate_sim);
    tools_end();
}
char* status_to_str(gs_status_e status){
    switch(status){
        case GS_OPENED: return "OPENED";
        case GS_CLOSED: return "CLOSED";
        case GS_MOVING: return "MOVING";
        case GS_STOPPED: return "STOPPED";
        default: return "UNKNOWN";
    }
}

char* event_to_str(gs_sm_event_e event){
    switch(event){
        case OPEN_COMMAND: return "OPEN_COMMAND";
        case CLOSE_COMMAND: return "CLOSE_COMMAND";
        case DUAL_COMMAND: return "DUAL_COMMAND";
        case STOP_COMMAND: return "STOP_COMMAND";
        case OPEN_END: return "OPEN_END";
        case CLOSE_END: return "CLOSE_END";
        default: return "UNKNOWN";
    }
}

char* state_to_str(gs_sm_state_e state){
    switch(state){
        case IDLE: return "IDLE";
        case OPENING: return "OPENING";
        case CLOSING: return "CLOSING";
        case STOPPED: return "STOPPED";
        default: return "UNKNOWN";
    }
}

void print_header(gs_s* gate_sim){
    cursor_up(10);
    printf("\r\n========================================\r\n");
    printf(">> FMS STATE:   %s            \r\n", state_to_str(gate_sim -> fsm.state));
    printf(">> FMS EVENT:   %s            \r\n", event_to_str(gate_sim -> fsm.event));
    printf(">> GATE STATUS: %s            \r\n", status_to_str(gate_sim -> status));
    printf("========================================\r\n\r\n");
}
uint8_t get_proportion(gs_s* gate_sim){
    return((uint8_t)(gate_sim -> counter.get_count() * 100.0 / TIME_UPPER_LIMIT ));
}

void draw_door(uint8_t proportion){
    uint8_t door_position;
    if(DRAW_TOOL_SCREEN_SIZE == 100){
        door_position = proportion;
    }else{
        door_position = (proportion/100.0) * DRAW_TOOL_SCREEN_SIZE;
    }
    cursor_up(0);
    printf("||\r\n");
    for(uint8_t i = 0; i < door_position; i++){
        cursor_up(0);
        cursor_right(i+2);
        printf("=\r\n");
    }

    cursor_up(0);
    cursor_right(door_position+3);
    if(proportion == 100){
        printf("CLOSED\r\n");
    }else{
        printf("> %3d%% \r\n", proportion);
        for(volatile uint8_t j = door_position + 8; j <= DRAW_TOOL_SCREEN_SIZE; j++){
            cursor_up(0);
            cursor_right(j+1);
            printf(" \r\n");
        }
    }
    cursor_up(0);
    cursor_right(DRAW_TOOL_SCREEN_SIZE+1);
    if(proportion == 100){
        printf("|| CLOSED \r\n");
    }else if (proportion == 0){
        printf("|| OPEN   \r\n");
    }else{
        printf("|| OPENING\r\n");
    }
}