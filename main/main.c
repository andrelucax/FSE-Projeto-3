#include <stdio.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "string.h"
#include "cJSON.h"
#include "esp_log.h"

#include "wifi.h"
#include "http_client.h"
#include "led.h"

#define BUFFER_SIZE 1024

#define IPSTACKKEY      CONFIG_IP_STACK_KEY
#define WEATHEMAPKEY      CONFIG_OPEN_WEATHER_MAP_KEY

xSemaphoreHandle conexaoWifiSemaphore;
xSemaphoreHandle httpSemaphore;
xSemaphoreHandle ledSemaphore;
int connectToWifi = 0;
// double latitude = 0;
// double longitude = 0;
char *response_buffer;
int response_size = 0;

void RealizaHTTPRequest(void * params)
{
  while(true)
  {
    if(xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY))
    {
      led_blink();
      if (response_size == 0)
      {
          response_buffer = (char *)malloc(BUFFER_SIZE);
          if (response_buffer == NULL)
          {
              ESP_LOGE("malloc", "Memory alloc error");
              vTaskDelay(500 / portTICK_PERIOD_MS);
              continue;
          }
      }

      ESP_LOGI("Main Task", "Realiza HTTP Request");
      char url[255] = "http://api.ipstack.com/check?access_key=";
      strcat(url, IPSTACKKEY);
      strcat(url, "&output=json");
      http_request(url);
      if (response_size != 0)
      {
        response_buffer[response_size] = '\0';
        cJSON * json = cJSON_Parse (response_buffer);
        double latitude = cJSON_GetObjectItemCaseSensitive(json, "latitude")->valuedouble;
        double longitude = cJSON_GetObjectItemCaseSensitive(json, "longitude")->valuedouble;
        cJSON_Delete(json);
        // printf("%lf %lf\n", latitude, longitude);
        free(response_buffer);
        response_buffer = NULL;
        response_size = 0;

        sprintf(url, "http://api.openweathermap.org/data/2.5/weather?lat=%lf&lon=%lf&appid=%s", latitude, longitude, WEATHEMAPKEY);
        
        if (response_size == 0)
        {
            response_buffer = (char *)malloc(BUFFER_SIZE);
            if (response_buffer == NULL)
            {
                ESP_LOGE("malloc", "Memory alloc error");
                vTaskDelay(500 / portTICK_PERIOD_MS);
                continue;
            }
        }
        
        http_request(url);

        if (response_size != 0)
        {
          response_buffer[response_size] = '\0';
          cJSON *json = cJSON_Parse (response_buffer);
          cJSON *mainwt = cJSON_GetObjectItemCaseSensitive(json, "main");
          double temperature = cJSON_GetObjectItemCaseSensitive(mainwt, "temp")->valuedouble;
          double minTemperature = cJSON_GetObjectItemCaseSensitive(mainwt, "temp_min")->valuedouble;
          double maxTemperature = cJSON_GetObjectItemCaseSensitive(mainwt, "temp_max")->valuedouble;
          int humidity = cJSON_GetObjectItemCaseSensitive(mainwt, "humidity")->valueint;
          // cJSON_Delete(mainwt);
          cJSON_Delete(json);
          printf("Temperatura atual: %.2lf\n", temperature - 273);
          printf("Temperatura minima: %.2lf\n", minTemperature - 273);
          printf("Temperatura maxima: %.2lf\n", maxTemperature - 273);
          printf("Humidade: %d %%\n", humidity);
          free(response_buffer);
          response_buffer = NULL;
          response_size = 0;
        }

      }

      xSemaphoreGive(conexaoWifiSemaphore);
    }

    vTaskDelay(1000 * 60 * 5 / portTICK_PERIOD_MS);
  }
}

void ledHandler(void * params)
{
  while(true)
  {
    if(xSemaphoreTake(ledSemaphore, portMAX_DELAY)){
        if(connectToWifi)
        {
            led_stay_on();
        }
        else{
            led_blink();
        }
    }
    xSemaphoreGive(ledSemaphore);
  }
}

// void ledHandler(void * params)
// {
//   while(true)
//   {
//     if(xSemaphoreTake(ledSemaphore, portMAX_DELAY)){
//         if(connectToWifi)
//         {
//             led_stay_on();
//         }
//         else{
//             led_blink();
//         }
//     }
//     xSemaphoreGive(ledSemaphore);
//   }
// }

void app_main(void)
{
    led_init();
    ledSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(ledSemaphore);
    xTaskCreate(&ledHandler,  "Led handler", 4096, NULL, 1, NULL);

    // Inicializa o NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    conexaoWifiSemaphore = xSemaphoreCreateBinary();
    httpSemaphore = xSemaphoreCreateBinary();
    wifi_start();

    xTaskCreate(&RealizaHTTPRequest,  "Processa HTTP", 4096*2, NULL, 1, NULL);

    
}
