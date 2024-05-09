#ifndef ___TOOLTS_H
#define ___TOOLTS_H
#include "main.h"
#include <stdbool.h>

typedef struct sw_s sw_s;

typedef enum{
	SW_ERR = -1,
	SW_PRESSED = GPIO_PIN_RESET,
	SW_RELEASED = GPIO_PIN_SET
}sw_state_e;

typedef enum{
	SW1 = SW_1_Pin,
	SW2 = SW_2_Pin,
	SW3 = EXT_INT_Pin
}sw_e;

typedef enum {
	LED_BLUE1,
	LED_BLUE2,
	LED_BLUE3,
	LED_GREEN
}led_e;

typedef enum{
	ON = GPIO_PIN_SET,
	OFF = GPIO_PIN_RESET,
	TOOGLE = 3
}led_state_e;

void tools_init();
void tools_run();
void tools_end();

bool led_switch(led_e led, led_state_e state);
sw_s* sw_init(sw_e new_id, char* new_name, sw_state_e new_state, uint8_t new_locked_cnt);
sw_state_e get_switch_state(sw_s* sw);

void clear_screen();
void cursor_to_origin();
void cursor_up(uint8_t step);
void cursor_right(uint8_t step);
#endif