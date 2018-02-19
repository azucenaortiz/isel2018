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

//estados
enum fsm_state {
  DESARMADA,
  ARMADA,
  DIGITO_1,
  DIGITO_2,
  DIGITO_3,
  DIGITO_4,
  DIGITO_5,
  DIGITO_6
};

#define ETS_GPIO_INTR_ENABLE() \
_xt_isr_unmask(1 << ETS_GPIO_INUM)
 #define ETS_GPIO_INTR_DISABLE() \
_xt_isr_mask(1 << ETS_GPIO_INUM)

int armed (fsm_t* );
void led_on (fsm_t* );
void led_off (fsm_t*);
int presence(fsm_t*);
int disarm(fsm_t*);

void isr_gpio(void* arg) {
 static portTickType xLastISRTick0 = 0;
 uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
 portTickType now = xTaskGetTickCount ();
 if (status & (BIT(0))) {
   if (now > xLastISRTick0) {
     xLastISRTick0 = now + 150/portTICK_RATE_MS;
     int done0 = 1;
    }
  }
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);
}

//definicion de funciones
int button_pressed(fsm_t*);
int function_timeout(fsm_t*);
int intruso(fsm_t*);
void led_on(fsm_t*);
void led_off(fsm_t*);
//void inter(void*);

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

int button_pressed (fsm_t* this) {
  return !GPIO_INPUT_GET(0);
}

int function_timeout (fsm_t *this) {
  portTickType now = xTaskGetTickCount();
   vTaskDelayUntil(&now, 1000/portTICK_RATE_MS);
   return 1;
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
  {DESARMADA, button_pressed, DIGITO_1, led_off},
  {DIGITO_1, function_timeout, DESARMADA, NULL},
  {DIGITO_1, button_pressed, DIGITO_2, NULL},
  {DIGITO_2, function_timeout, DESARMADA, NULL},
  {DIGITO_2, button_pressed, DIGITO_3, NULL},
  {DIGITO_3, function_timeout, DESARMADA, NULL},
  {DIGITO_3, button_pressed, ARMADA, NULL},

  {ARMADA, intruso, ARMADA, led_on},

  {ARMADA, button_pressed, DIGITO_4, NULL},
  {DIGITO_4, function_timeout, ARMADA, NULL},
  {DIGITO_4, button_pressed, DIGITO_5, NULL},
  {DIGITO_5, function_timeout, ARMADA, NULL},
  {DIGITO_5, button_pressed, DIGITO_6, NULL},
  {DIGITO_6, function_timeout, ARMADA, NULL},
  {DIGITO_6, button_pressed, DESARMADA, led_off},

  {-1, NULL, -1, NULL},
};

void inter(void* ignore){
  GPIO_ConfigTypeDef io_conf, i15_conf;
  io_conf.GPIO_IntrType = GPIO_PIN_INTR_NEGEDGE;
  io_conf.GPIO_Mode = GPIO_Mode_Input;
  io_conf.GPIO_Pin = BIT(0);
  io_conf.GPIO_Pullup = GPIO_PullUp_EN;
  gpio_config(&io_conf);
  i15_conf.GPIO_IntrType = GPIO_PIN_INTR_POSEDGE;
  i15_conf.GPIO_Mode = GPIO_Mode_Input;
  i15_conf.GPIO_Pin = BIT(15);
  i15_conf.GPIO_Pullup = GPIO_PullUp_EN;
  gpio_config(&i15_conf);

 fsm_t* fsm = fsm_new(alarm);
 led_off(fsm);
 portTickType xLastWakeTime;
 //
 gpio_intr_handler_register((void*)isr_gpio, NULL);
 gpio_pin_intr_state_set(0, GPIO_PIN_INTR_NEGEDGE);
 gpio_pin_intr_state_set(15, GPIO_PIN_INTR_POSEDGE);
 //
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
