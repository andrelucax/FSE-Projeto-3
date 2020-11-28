#include "sdkconfig.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"

#define LED 2

extern int connectToWifi;

void led_init(){
    gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
}

void led_blink(){
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(LED, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(LED, 1);
}

void led_stay_on(){
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(LED, 1);
}