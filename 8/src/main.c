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
#define ANTIRREBOTES 150/portTICK_RATE_MS
#define SEGUNDO 1000/portTICK_RATE_MS

int code_index = 0;
int code_inserted[3];
int ok_code[3] = {1, 2, 3};
volatile int done0 = 0;
volatile int done15 = 0;
portTickType next_timeout;

enum fsm_state {
  DESARMADA,
  ARMADA,
};

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

#define ETS_GPIO_INTR_ENABLE() \
_xt_isr_unmask(1 << ETS_GPIO_INUM)
 #define ETS_GPIO_INTR_DISABLE() \
_xt_isr_mask(1 << ETS_GPIO_INUM)

void isr_gpio(void* arg) {
 static portTickType xLastISRTick0 = 0;
 uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
 portTickType now = xTaskGetTickCount ();
 if (status & (BIT(0))) {
   if (now > xLastISRTick0) {
     xLastISRTick0 = now + ANTIRREBOTES;
     int done0 = 1;
    }
  }
  if (status & (BIT(15))) {
    if (now > xLastISRTick0) {
      xLastISRTick0 = now + ANTIRREBOTES;
      int done15 = 1;
     }
   }
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);
}

int intruso (fsm_t* shelf) {
  return done15;
}
void led_on (fsm_t *shelf) {
    GPIO_OUTPUT_SET(2, 0);
}
void led_off (fsm_t *shelf) {
    GPIO_OUTPUT_SET(2, 1);
}

int mirar_flags (fsm_t* shelf) {
  if(code_index > 2 || code_inserted[code_index] > 10)
    return 0;
  return done0;
}
void update_code (fsm_t* shelf) {
  code_inserted[code_index]++;
  done0 = 0;
  next_timeout = xTaskGetTickCount() + SEGUNDO;
}

int function_timeout (fsm_t *shelf) {
  portTickType now = xTaskGetTickCount();
  if(now >= next_timeout)
    return 1;
  return 0;
}
void next_index (fsm_t* shelf) {
  code_index++;
  next_timeout = 0xFFFFFFFF;
}

int codigo_incorrecto (fsm_t* shelf) {
  int i;
  for(i = 0; i < 3; i++){
    if (code_inserted[i] != ok_code[i]){
      return 1;
    }
  }
  return 0;
}
int codigo_correcto (fsm_t* shelf) {
  led_off(shelf);
  int i;
  for(i = 0; i < 3; i++){
    if (code_inserted[i] == ok_code[i]){
      continue;
    } else {
      return 0;
    }
  }
  return 1;
}
void limpiar_flags (fsm_t* shelf) {
  done0 = 0;
  done15 = 0;
  int i;
  for(i = 0; i < 3; i++){
    code_inserted[i] = 0;
  }
}

static fsm_trans_t alarm[] = {
  {DESARMADA, mirar_flags, DESARMADA, update_code},
  {DESARMADA, function_timeout, DESARMADA, next_index},
  {DESARMADA, codigo_incorrecto, DESARMADA, limpiar_flags},
  {DESARMADA, codigo_correcto, ARMADA, limpiar_flags},
  {ARMADA, mirar_flags, ARMADA, update_code},
  {ARMADA, function_timeout, ARMADA, next_index},
  {ARMADA, intruso, ARMADA, led_on},
  {ARMADA, codigo_incorrecto, ARMADA, limpiar_flags},
  {ARMADA, codigo_correcto, DESARMADA, limpiar_flags},
  {-1, NULL, -1, NULL},
};

void inter(void* ignore){
  GPIO_ConfigTypeDef i0_conf;
  i0_conf.GPIO_IntrType = GPIO_PIN_INTR_NEGEDGE;
  i0_conf.GPIO_Mode = GPIO_Mode_Input;
  i0_conf.GPIO_Pin = BIT(0);
  i0_conf.GPIO_Pullup = GPIO_PullUp_EN;
  gpio_config(&i0_conf);
  GPIO_ConfigTypeDef i15_conf;
  i15_conf.GPIO_IntrType = GPIO_PIN_INTR_POSEDGE;
  i15_conf.GPIO_Mode = GPIO_Mode_Input;
  i15_conf.GPIO_Pin = BIT(15);
  i15_conf.GPIO_Pullup = GPIO_PullUp_EN;
  gpio_config(&i15_conf);

 fsm_t* fsm = fsm_new(alarm);
 led_off(fsm);
 portTickType xLastWakeTime;
 gpio_intr_handler_register((void*)isr_gpio, NULL);
 gpio_pin_intr_state_set(0, GPIO_PIN_INTR_NEGEDGE);
 gpio_pin_intr_state_set(15, GPIO_PIN_INTR_POSEDGE);
 ETS_GPIO_INTR_ENABLE();
  while(true) {
      xLastWakeTime = xTaskGetTickCount();
      fsm_fire(fsm);
      vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK);
  }
}

void user_init(void)
{
    xTaskCreate(&inter, "startup", 2048, NULL, 1, NULL);
}
