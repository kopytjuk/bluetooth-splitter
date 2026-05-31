#include <driver/i2s.h>
#include "BluetoothA2DPSource.h"

// --- Configuration ---
const char* bluetooth_device_name = "Your Headphones Name"; // Change to your remote BT speaker/headphones name

// Define the ESP32 hardware pins
#define I2S_BCLK_PIN  26  // Connected to Pi 5 GPIO 18
#define I2S_LRCLK_PIN 25  // Connected to Pi 5 GPIO 19
#define I2S_DATA_PIN  22  // Connected to Pi 5 GPIO 21
#define LED_PIN       15  // NodeMCU D15

// Define the I2S port (ESP32 has port 0 and port 1)
#define I2S_PORT_NUM  I2S_NUM_0

BluetoothA2DPSource a2dp_source;

int32_t get_i2s_data(Frame *data, int32_t len) {
  size_t bytes_read = 0;
  // 'len' is the number of stereo frames requested.
  esp_err_t result = i2s_read(I2S_PORT_NUM, data, len * sizeof(Frame), &bytes_read, portMAX_DELAY);
  if (result != ESP_OK) {
    return 0;
  }
  return (int32_t)(bytes_read / sizeof(Frame));
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- ESP32 I2S audio receiver + Bluetooth A2DP source starting ---");

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_SLAVE | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK_PIN,
    .ws_io_num = I2S_LRCLK_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_DATA_PIN
  };

  esp_err_t err = i2s_driver_install(I2S_PORT_NUM, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Error installing I2S driver: %d\n", err);
    while (true);
  }

  err = i2s_set_pin(I2S_PORT_NUM, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("Error assigning I2S pins: %d\n", err);
    while (true);
  }

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.print("Connecting to Bluetooth device: ");
  Serial.println(bluetooth_device_name);
  a2dp_source.start(bluetooth_device_name, get_i2s_data);
}

void loop() {
  delay(1000);
}
