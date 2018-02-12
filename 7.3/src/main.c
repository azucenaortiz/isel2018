#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include <string.h>
#include <stdio.h>

#define PIN_D4 2

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

const char* morse(char c)
{
  static const char* morse_ch[] = {
    "._ ", /*a*/
    "_... ", /*b*/
    "_._. ", /*c*/
    "_.. ", /*d*/
    ". ", /*e*/
    ".._. ", /*f*/
    "__. ", /*g*/
    ".... ", /*h*/
    ".. ", /*i*/
    ".___ ", /*j*/
    "_._ ", /*k*/
    "._.. ", /*l*/
    "__ ", /*m*/
    "_. ", /*n*/
    "___ ", /*o*/
    ".__. ", /*p*/
    "__._ ", /*q*/
    "._. ", /*r*/
    "... ", /*s*/
    "_ ", /*t*/
    ".._ ", /*u*/
    "..._ ", /*v*/
    ".__ ", /*w*/
    "_.._ ", /*x*/
    "_.__ ", /*y*/
    "__.. ", /*z*/
  };
  return morse_ch[c - 'a'];
}

int str2morse(char* buf, int n, const char* str)
{
int n_caracteres = 0;

  while(*str){
    if (*str == ' '){
      morse_send("     ");
    } else {
      buf = morse(*str);
      morse_send(buf);
    }
    n_caracteres++;
    str++;
  }

  return n_caracteres;
}

void morse_send(const char* msg)
{
  switch(*msg){
    case '.':
      GPIO_OUTPUT_SET(PIN_D4,0);
      vTaskDelay(250/portTICK_RATE_MS);
      GPIO_OUTPUT_SET(PIN_D4,1);
      vTaskDelay(250/portTICK_RATE_MS);
      break;
    case '_':
      GPIO_OUTPUT_SET(PIN_D4,0);
      vTaskDelay(750/portTICK_RATE_MS);
      GPIO_OUTPUT_SET(PIN_D4,1);
      vTaskDelay(250/portTICK_RATE_MS);
      break;
    case ' ':
      GPIO_OUTPUT_SET(PIN_D4,1);
      vTaskDelay(500/portTICK_RATE_MS);
      break;
      case '\0':
      GPIO_OUTPUT_SET(PIN_D4,1);
      return;
  }
  morse_send(++msg);
}

void morse_receive(void* ignore)
{
  const char* str = "hola mundo";
  const char* buf = str;
  int n = 70;

  str2morse(buf, n, str);
  morse_send(buf);

  vTaskDelete(NULL);
}

void user_init(void)
{
    xTaskCreate(&morse_receive, "startup", 2048, NULL, 1, NULL);
}
