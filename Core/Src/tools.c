#include "tools.h"
#include "main.h"
#include "gate_simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO

typedef struct sw_s{
	sw_e id;
	char name[15];
	sw_state_e state;
	uint8_t locked_cnt;
}sw_s;

sw_s *sw1, *sw2, *sw3;

struct counter_s{
    uint16_t counter;
    bool direction;
    bool locked;
};

struct counter_s counter;

void change_direction(bool direction){
    counter.direction = direction;    
}

void reset(uint16_t value){
    counter.counter = value;
}

uint16_t get_count(){
    return counter.counter;
}

void run_counter(){
    counter.locked = false;
}

void stop_counter(){
    counter.locked = true;
}

sw_state_e get_switch_state(sw_s* sw){
    return sw->state;
}

extern UART_HandleTypeDef huart1;

int _write(int file, char *data, int len){
	 if ((file != STDOUT_FILENO) && (file != STDERR_FILENO)){
			errno = EBADF;
			return -1;
	 }
	 HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, (uint8_t*)data, len, 1000);

	 return (status == HAL_OK ? len : 0);
}

sw_s* sw_init(sw_e new_id, char* new_name, sw_state_e new_state, uint8_t new_locked_cnt){
    sw_s *new_sw = (sw_s*)malloc(sizeof(sw_s));

    new_sw -> id = new_id;
    sprintf(new_sw -> name, "%s", new_name); // strcpy(new_sw -> name, new_name
    new_sw -> state = new_state;
    new_sw -> locked_cnt = new_locked_cnt;

    return new_sw;
}

void switch_get(sw_s *sw){
	if(sw->locked_cnt == 0){
		switch(sw->id){
			case(SW1):{
				sw -> state = HAL_GPIO_ReadPin(GPIOA, SW_1_Pin);
				break;
			}
			case(SW2):{
				sw -> state = HAL_GPIO_ReadPin(GPIOB, SW_2_Pin);
				break;
			}
			case(SW3):{
				sw -> state = HAL_GPIO_ReadPin(EXT_INT_GPIO_Port, EXT_INT_Pin);
				break;
			}
			default: return SW_ERR; break;
		}
		if( sw -> state == SW_PRESSED ){
			// printf("%s pressed\r\n", sw->name);
			sw -> locked_cnt = 200;
		}
	}
}

bool led_switch(led_e led, led_state_e state){
	if(state > TOOGLE){ return false; }

	switch(led){
		case(LED_BLUE1):
			state == TOOGLE ? HAL_GPIO_TogglePin(GPIOB, B_LED1_Pin) : HAL_GPIO_WritePin(GPIOB, B_LED1_Pin, state);
			break;
		
		case(LED_BLUE2):
			state == TOOGLE ? HAL_GPIO_TogglePin(GPIOB, B_LED2_Pin) : HAL_GPIO_WritePin(GPIOB, B_LED2_Pin, state);
			break;
		
		case(LED_BLUE3):
			state == TOOGLE ? HAL_GPIO_TogglePin(GPIOB, B_LED3_Pin) : HAL_GPIO_WritePin(GPIOB, B_LED3_Pin, state);
			break;
		
		case(LED_GREEN):
			state == TOOGLE ? HAL_GPIO_TogglePin(GPIOB, G_LED_Pin) : HAL_GPIO_WritePin(GPIOB, G_LED_Pin, state);
			break;
		default: return false; break;
	}
	return true;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim){

	if(sw1 -> locked_cnt > 0){ sw1 -> locked_cnt--; }
	if(sw2 -> locked_cnt > 0){ sw2 -> locked_cnt--; }
	if(sw3 -> locked_cnt > 0){ sw3 -> locked_cnt--; }

	if(htim->Instance == TIM2){
        if(counter.locked == false){
            if(counter.direction == true){
                if(counter.counter < TIME_UPPER_LIMIT){
                    counter.counter++;
                }
            }else{
                if(counter.counter > TIME_LOWER_LIMIT){
                    counter.counter--;
                }
            }
        }
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == EXT_INT_Pin){
		switch_get(sw3);
	}
}

void tools_run(){
    switch_get(sw1);
    switch_get(sw2);
}

void tools_init(){
    printf("tools_init\r\n");

	led_switch(LED_BLUE1, OFF);
	led_switch(LED_BLUE2, ON);
	led_switch(LED_BLUE3, OFF);
	led_switch(LED_GREEN, ON);

}

void tools_end(){
    printf("tools_end\r\n");
    free(sw1);
    free(sw2);
    free(sw3);
}

void clear_screen(){
	printf("\033[2J");
	printf("\033[0;0H");
}

void cursor_to_origin(){
	printf("\033[0;0H");
}

void cursor_up(uint8_t step){
	printf("\033[%dA", step);
}

void cursor_right(uint8_t step){
	printf("\033[%dC", step);
}