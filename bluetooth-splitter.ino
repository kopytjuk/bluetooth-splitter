#include <driver/i2s.h>
#include "BluetoothA2DPSource.h"

// --- Configuration ---
const char* bluetooth_device_name = "SoundCore 2"; 

// Define the ESP32 hardware pins
#define I2S_BCLK_PIN  26  // Connected to Pi 5 GPIO 18
#define I2S_LRCLK_PIN 25  // Connected to Pi 5 GPIO 19
#define I2S_DATA_PIN  22  // Connected to Pi 5 GPIO 21
#define LED_PIN       15  // NodeMCU D15

#define I2S_PORT_NUM  I2S_NUM_0

BluetoothA2DPSource a2dp_source;

// Define a structural mirror for the Pi 5's S32_LE hardware layout
struct Raw32Frame {
  int32_t left;
  int32_t right;
};

int32_t get_i2s_data(Frame *data, int32_t len) {
  size_t bytes_read = 0;
  
  // Create a local buffer to pull raw 32-bit frames off the hardware I2S line
  Raw32Frame temp_buffer[len];

  // Request the full 32-bit frame byte allocation (8 bytes per frame)
  esp_err_t result = i2s_read(I2S_PORT_NUM, temp_buffer, len * sizeof(Raw32Frame), &bytes_read, portMAX_DELAY);
  
  if (result != ESP_OK || bytes_read == 0) {
    return 0;
  }

  // Calculate exactly how many full 32-bit frames were extracted
  int32_t frames_read = (int32_t)(bytes_read / sizeof(Raw32Frame));

  // Cast the complex Frame pointer into a flat array of standard 16-bit integers.
  // Index [2 * i] is Left, index [2 * i + 1] is Right.
  int16_t *channels = (int16_t*)data;

  // Downsample 32-bit data to 16-bit data on the fly
  for (int32_t i = 0; i < frames_read; i++) {
    channels[2 * i]     = (int16_t)(temp_buffer[i].left >> 16);  // Left channel
    channels[2 * i + 1] = (int16_t)(temp_buffer[i].right >> 16); // Right channel
  }

  // Tell the Bluetooth source library exactly how many frames are packed
  return frames_read;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- ESP32 I2S audio receiver + Bluetooth A2DP source starting ---");

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_SLAVE | I2S_MODE_RX),
    .sample_rate = 48000,                           
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,   // Confirmed via Pi 5 hw_params
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false                               // Slave mode ignores internal APLL clocks
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
  
  // Pass the data acquisition callback handler
  a2dp_source.start(bluetooth_device_name, get_i2s_data);
}

void loop() {
  // Flash status LED to indicate execution is running fine loop-side
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
}