#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// ===== WIFI AP =====
const char* ssid = "ESP32-CAM";
const char* password = "12345678";

// ===== CAMERA PINS (AI THINKER) =====
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ===== STREAM HANDLER =====
static esp_err_t stream_handler(httpd_req_t *req) {

  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;

  res = httpd_resp_set_type(req,
    "multipart/x-mixed-replace; boundary=frame");

  if (res != ESP_OK) return res;

  while (true) {

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return ESP_FAIL;
    }

    char header[64];
    int hlen = snprintf(header, sizeof(header),
      "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
      fb->len);

    res = httpd_resp_send_chunk(req, header, hlen);

    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req,
        (const char *)fb->buf, fb->len);
    }

    esp_camera_fb_return(fb);

    if (res != ESP_OK) break;
  }

  return res;
}

// ===== FULLSCREEN WEB PAGE =====
static esp_err_t index_handler(httpd_req_t *req) {

  const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <title>ESP32-CAM Live</title>

  <style>
    body {
      margin: 0;
      background: black;
      overflow: hidden;
      font-family: Arial;
    }

    h2 {
      position: absolute;
      top: 10px;
      width: 100%;
      text-align: center;
      color: white;
      font-size: 18px;
      z-index: 10;
      margin: 0;
    }

    .stream {
      width: 100vw;
      height: 100vh;
      object-fit: contain;
      background: black;
    }

    img {
      width: 100vw;
      height: 100vh;
      object-fit: contain;
    }
  </style>
</head>

<body>

  <h2>ESP32-CAM LIVE STREAM</h2>

  <img src="/stream">

</body>
</html>
)rawliteral";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html, strlen(html));

  return ESP_OK;
}

// ===== START SERVER =====
void startCameraServer() {

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = NULL;

  if (httpd_start(&server, &config) == ESP_OK) {

    httpd_uri_t index_uri = {
      .uri       = "/",
      .method    = HTTP_GET,
      .handler   = index_handler,
      .user_ctx  = NULL
    };

    httpd_uri_t stream_uri = {
      .uri       = "/stream",
      .method    = HTTP_GET,
      .handler   = stream_handler,
      .user_ctx  = NULL
    };

    httpd_register_uri_handler(server, &index_uri);
    httpd_register_uri_handler(server, &stream_uri);
  }
}

// ===== SETUP =====
void setup() {

  Serial.begin(115200);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;   // good FPV balance
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QQVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  // ===== START WIFI AP =====
  WiFi.softAP(ssid, password);

  Serial.println("\nWiFi created!");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.println("Open: http://192.168.4.1");

  startCameraServer();
}

// ===== LOOP =====
void loop() {
}