
#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>

// ===========================
// Modelo de cámara
// ===========================
#include "board_config.h"

// ===========================
// WiFi
// ===========================
const char *ssid = "IoT";
const char *password = "laboratorioIOT";

void startCameraServer();
void setupLedFlash();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // ===========================
  // Configuración de cámara
  // ===========================
  camera_config_t config = {};

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

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

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  // ===========================
  // Máxima resolución
  // ===========================
  config.xclk_freq_hz = 20000000;

  // UXGA = 1600 x 1200
  config.frame_size = FRAMESIZE_UXGA;

  // JPEG para transmisión por WiFi
  config.pixel_format = PIXFORMAT_JPEG;

  // Obtener el cuadro más reciente
  config.grab_mode = CAMERA_GRAB_LATEST;

  // Usar memoria PSRAM
  config.fb_location = CAMERA_FB_IN_PSRAM;

  // Calidad JPEG alta:
  // menor número = mayor calidad y archivos más grandes
  config.jpeg_quality = 10;

  // Dos buffers si la memoria lo permite
  config.fb_count = 2;

  // ===========================
  // Inicializar cámara
  // ===========================
  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {
    Serial.printf(
      "Error al inicializar la cámara: 0x%x\n",
      err
    );
    Serial.println(
      "Revisa la PSRAM, el modelo y las conexiones."
    );
    return;
  }

  Serial.println("CAMARA INICIALIZADA CORRECTAMENTE");

  // ===========================
  // Configuración del sensor
  // ===========================
  sensor_t *s = esp_camera_sensor_get();

  if (s == nullptr) {
    Serial.println("No se pudo obtener el sensor.");
    return;
  }

  // Aplicar UXGA al sensor
  s->set_framesize(s, FRAMESIZE_UXGA);

  // Ajustes específicos para OV3660
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }

  // Orientación para otros modelos
#if defined(CAMERA_MODEL_M5STACK_WIDE) || \
    defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

  // ===========================
  // LED de flash
  // ===========================
#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  // ===========================
  // Conectar WiFi
  // ===========================
  WiFi.begin(ssid, password);

  // Desactivar ahorro de energía WiFi
  WiFi.setSleep(false);

  Serial.println();
  Serial.print("Conectando al WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  // ===========================
  // Iniciar servidor
  // ===========================
  startCameraServer();

  Serial.println("Camara lista.");
  Serial.print("Pagina web: http://");
  Serial.println(WiFi.localIP());

  Serial.print("Stream: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/stream");

  Serial.println("Resolucion solicitada: UXGA 1600x1200");
}

void loop() {
  delay(10000);
}
