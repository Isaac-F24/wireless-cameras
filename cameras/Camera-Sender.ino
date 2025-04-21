#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// Select your camera model
#define CAMERA_MODEL_XIAO_ESP32S3
#include "camera_pins.h"

// ====== Wi‑Fi & backend settings ======
const char *ssid       = "Wifi";
const char *password   = "password";
const char* camID = "bedroom";
String url = String("http://") + "IP:Port" + "/upload/" + camID;

// ====== forward declarations ======
void frameSenderTask(void *pvParameters);

void setup() {
  Serial.begin(115200);

  // ---- Wi‑Fi ----
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println("\nWi‑Fi connected → " + WiFi.localIP().toString());

  // ---- Camera ----
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size   = FRAMESIZE_VGA;   // 640×480
    config.jpeg_quality = 12;              // 0‑63 (lower = better)
    config.fb_count     = 2;
  } else {
    config.frame_size   = FRAMESIZE_QVGA;  // 320×240
    config.jpeg_quality = 15;
    config.fb_count     = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    ESP.restart();
  }

  // ---- Spawn a FreeRTOS task that pushes frames ----
  xTaskCreatePinnedToCore(
      frameSenderTask,
      "FrameSender",
      8192,        // stack
      nullptr,
      1,           // priority (idle+1)
      nullptr,
      1            // run on core 1 so Wi‑Fi (core0) keeps breathing
  );
}

void loop() {
  delay(10000);
}

// =================================================================
//                     Frame‑pushing task
// =================================================================
void frameSenderTask(void *pvParameters) {
  HTTPClient http;

  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      continue;
    }

    http.begin(url);
    http.addHeader("Content-Type", "application/octet-stream");   // raw JPEG
    int httpCode = http.POST(fb->buf, fb->len);
    if (httpCode != 204) {
      Serial.printf("POST failed: %d\n", httpCode);
    }
    http.end();

    esp_camera_fb_return(fb);
    vTaskDelay(pdMS_TO_TICKS(100));   // ≈10 fps – adjust as you like
  }
}
