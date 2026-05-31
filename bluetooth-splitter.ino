#include <driver/i2s.h>

// Define the ESP32 hardware pins
#define I2S_BCLK_PIN  26  // Connected to Pi 5 GPIO 18
#define I2S_LRCLK_PIN 25  // Connected to Pi 5 GPIO 19
#define I2S_DATA_PIN  22  // Connected to Pi 5 GPIO 21

// Define the I2S port (ESP32 has port 0 and port 1)
#define I2S_PORT_NUM  I2S_NUM_0

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- ESP32 I2S audio receiver starting ---");

  // 1. Define I2S configuration
  // The iPad delivers 44.1 kHz 16-bit stereo via Bluetooth by default.
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_SLAVE | I2S_MODE_RX), // ESP32 is slave and receives (RX)
    .sample_rate = 44100,                               // Sample rate: 44.1 kHz (CD quality)
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,       // Resolution: 16-bit per sample
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,       // Channel format: stereo (right/left)
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,   // Standard I2S protocol
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,           // Interrupt priority
    .dma_buf_count = 8,                                 // Number of DMA buffers
    .dma_buf_len = 64,                                  // Size of each buffer
    .use_apll = false                                   // Audio PLL not needed as slave
  };

  // 2. Assign I2S pins
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK_PIN,
    .ws_io_num = I2S_LRCLK_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,                  // We do not send data out
    .data_in_num = I2S_DATA_PIN                         // Data input from Pi
  };

  // 3. Install driver
  esp_err_t err = i2s_driver_install(I2S_PORT_NUM, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Error installing I2S driver: %d\n", err);
    while (true); // stop on error
  }

  // 4. Activate pins
  err = i2s_set_pin(I2S_PORT_NUM, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("Error assigning I2S pins: %d\n", err);
    while (true);
  }

  Serial.println("I2S configured successfully. Waiting for audio clock from Raspberry Pi...");
}

void loop() {
  // Buffer for incoming audio data (256 bytes)
  uint8_t audio_buffer[256];
  size_t bytes_read = 0;

  // Read live data from the I2S bus
  // 'portMAX_DELAY' blocks the loop until data is pushed from the Pi.
  esp_err_t result = i2s_read(I2S_PORT_NUM, &audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);

  if (result == ESP_OK && bytes_read > 0) {
    
    // =========================================================================
    // YOUR RAW AUDIO DATA IS HERE!
    // =========================================================================
    // 'audio_buffer' now contains the PCM audio data from your iPad.
    // Since this is 16-bit data, every 2 bytes correspond to one sample.
    
    // Example for demonstration: every 500 received packets we print a status
    // message to the serial monitor to confirm the data flow.
    static int packet_counter = 0;
    packet_counter++;
    
    if (packet_counter >= 500) {
      Serial.printf("[INFO] Audio stream active. %d bytes read.\n", bytes_read);
      
      // Access the first 16-bit sample as an example (combine two bytes):
      int16_t sample = (audio_buffer[1] << 8) | audio_buffer[0];
      Serial.printf("Current raw amplitude value: %d\n", sample);
      
      packet_counter = 0;
    }
    
    // TIP: If you have attached a speaker DAC (e.g. MAX98357A) to the ESP32 over I2S,
    // you would forward the data here with 'i2s_write' to the second DAC port.

  }
}