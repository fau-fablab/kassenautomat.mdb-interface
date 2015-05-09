#include "./global.h"

// please update docs in main.c if you change something here
typedef enum {HOPPER_RESPONSE_BUSY='B', HOPPER_RESPONSE_SUCCESS='D', HOPPER_RESPONSE_EMPTY='E', HOPPER_RESPONSE_ALREADY_SENT='!'} hopperResponseEnum;
typedef enum {HOPPER_OKAY=0, HOPPER_ERR_SENSOR1=1, HOPPER_ERR_SENSOR2=2, HOPPER_ERR_SHORT_COIN_IMPULSE=3, HOPPER_ERR_UNEXPECTED_COIN=4, HOPPER_ERR_EARLY_COIN=5, HOPPER_ERR_UNEXPECTED_COIN_AT_COOLDOWN=6} hopperErrorEnum;



hopperErrorEnum hopper_get_error(void);
uint8_t hopper_busy(void);
uint8_t hopper_has_response(void);
void hopper_request_dispense(void);
hopperResponseEnum hopper_get_response(void);
void hopper_init(void);
void task_hopper(void);