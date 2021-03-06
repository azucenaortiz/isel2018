#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"

/*
#define LED 2
#define ARMAR/DESARMAR 0
#define INTRUSO 15
*/

#define PERIOD_TICK 100/portTICK_RATE_MS
volatile long timeout = 0;

//estados
enum fsm_state {
  DESARMADA,
  ARMADA
};

//definicion de funciones
int alarma_on(fsm_t*);
int alarma_off(fsm_t*);
int intruso(fsm_t*);
void led_on(fsm_t*);
void led_off(fsm_t*);
void inter(void*);

uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;
    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

int alarma_on (fsm_t* this) {
  if(GPIO_INPUT_GET(0) == 1){
      if (xTaskGetTickCount() < timeout) {
        timeout = xTaskGetTickCount() + 150/portTICK_RATE_MS;
        return 0;
      } else {
        timeout = xTaskGetTickCount() + 150/portTICK_RATE_MS;
        return 1;
      }
    }
    return 0;
}

int alarma_off (fsm_t *this) {
  if(GPIO_INPUT_GET(0) == 0){
      if (xTaskGetTickCount() < timeout) {
        timeout = xTaskGetTickCount() + 150/portTICK_RATE_MS;
        return 0;
      } else {
        timeout = xTaskGetTickCount() + 150/portTICK_RATE_MS;
        return 1;
      }
    }
    return 0;
}

int intruso (fsm_t* this) {
  return GPIO_INPUT_GET(15);
}

void led_on (fsm_t *this) {
    GPIO_OUTPUT_SET(2, 0);
}

void led_off (fsm_t *this) {
    GPIO_OUTPUT_SET(2, 1);
}


static fsm_trans_t alarm[] = {
  {DESARMADA, alarma_on, ARMADA, led_off},
  {ARMADA, alarma_off, DESARMADA, led_off},
  {ARMADA, intruso, ARMADA, led_on},
  {-1, NULL, -1, NULL},
};

void inter(void* ignore)
{
    fsm_t* fsm = fsm_new(alarm);
    led_off(fsm);
    portTickType xLastWakeTime;

    while(true) {
      xLastWakeTime = xTaskGetTickCount ();
      fsm_fire(fsm);
      vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK);
    }
}

void user_init(void)
{
    PIN_FUNC_SELECT(GPIO_PIN_REG_15, FUNC_GPIO15);
    xTaskCreate(&inter, "startup", 2048, NULL, 1, NULL);
}
