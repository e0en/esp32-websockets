#include "secret.h"
#include "wifi.h"
#include <driver/gpio.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>

esp_err_t websocket_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    ESP_LOGI("WS", "WebSocket connection established");

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    while (1) {
      char sensor_data[32];
      snprintf(sensor_data, sizeof(sensor_data), "{\"value\": %d}\n", 123);
      ws_pkt.payload = (uint8_t *)sensor_data;
      ws_pkt.len = strlen(sensor_data);
      ws_pkt.final = true;

      esp_err_t ret = httpd_ws_send_frame_async(
          req->handle, httpd_req_to_sockfd(req), &ws_pkt);

      if (ret != ESP_OK) {
        ESP_LOGW("WS", "Client disconnected or error occurred: %d", ret);
        break;
      }

      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
  return ESP_OK;
}

void webserver_task(void *pvParameters) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_uri_t ws_uri = {.uri = "/ws",
                          .method = HTTP_GET,
                          .handler = websocket_handler,
                          .is_websocket = true};

    httpd_register_uri_handler(server, &ws_uri);
    ESP_LOGI("HTTPD", "WebSocket server started");
  }
  vTaskDelete(NULL);
}

void led_task(void *pvParameters) {
  int cnt = 0;
  bool level = cnt % 2;
  while (1) {
    ESP_LOGI("LED", "%d", level);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_2, level);
    cnt++;
    level = cnt % 2;
  }
}

void app_main(void) {
  init_wifi(WIFI_SSID, WIFI_PASSWORD, true);

  uint64_t gpio_pin_sel = (1 << GPIO_NUM_2);
  gpio_config_t io_conf = {
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = gpio_pin_sel,
      .pull_down_en = 0,
      .pull_up_en = 0,
  };
  gpio_config(&io_conf);

  xTaskCreate(webserver_task, "webserver_task", 8192, NULL, 5, NULL);
  xTaskCreate(led_task, "led_task", 8192, NULL, 5, NULL);
}
