#include <driver/i2s.h>

// Definition der Hardware-Pins am ESP32
#define I2S_BCLK_PIN  26  // Verbunden mit Pi 5 GPIO 18
#define I2S_LRCLK_PIN 25  // Verbunden mit Pi 5 GPIO 19
#define I2S_DATA_PIN  22  // Verbunden mit Pi 5 GPIO 21

// Definition des I2S-Ports (ESP32 hat Port 0 und Port 1)
#define I2S_PORT_NUM  I2S_NUM_0

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- ESP32 I2S Audio-Empfänger startet ---");

  // 1. I2S-Profil definieren
  // Das iPad liefert via Bluetooth standardmäßig 44.1 kHz in 16-Bit Stereo.
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_SLAVE | I2S_MODE_RX), // ESP32 ist Slave und empfängt (RX)
    .sample_rate = 44100,                               // Samplerate: 44.1 kHz (CD-Qualität)
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,       // Auflösung: 16-Bit pro Sample
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,       // Kanal-Format: Stereo (Rechts/Links)
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,   // Standard I2S Protokoll
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,           // Interrupt-Priorität
    .dma_buf_count = 8,                                 // Anzahl der DMA-Buffer
    .dma_buf_len = 64,                                  // Größe der einzelnen Buffer
    .use_apll = false                                   // Audio-PLL wird als Slave nicht benötigt
  };

  // 2. I2S-Pins zuweisen
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK_PIN,
    .ws_io_num = I2S_LRCLK_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,                  // Wir senden keine Daten aus
    .data_in_num = I2S_DATA_PIN                         // Dateneingang vom Pi
  };

  // 3. Treiber installieren
  esp_err_t err = i2s_driver_install(I2S_PORT_NUM, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Fehler beim Installieren des I2S-Treibers: %d\n", err);
    while (true); // Stopp bei Fehler
  }

  // 4. Pins aktivieren
  err = i2s_set_pin(I2S_PORT_NUM, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("Fehler beim Zuweisen der I2S-Pins: %d\n", err);
    while (true);
  }

  Serial.println("I2S erfolgreich konfiguriert. Warte auf Audio-Takt vom Raspberry Pi...");
}

void loop() {
  // Buffer für die eintreffenden Audiodaten (256 Bytes)
  uint8_t audio_buffer[256];
  size_t bytes_read = 0;

  // Live-Daten vom I2S-Bus abrufen
  // 'portMAX_DELAY' blockiert die Schleife elegant, bis Daten vom Pi reingeschoben werden.
  esp_err_t result = i2s_read(I2S_PORT_NUM, &audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);

  if (result == ESP_OK && bytes_read > 0) {
    
    // =========================================================================
    // HIER LIEGEN DEINE ROHEN AUDIODATEN!
    // =========================================================================
    // 'audio_buffer' enthält jetzt die PCM-Audiodaten deines iPads.
    // Da es sich um 16-Bit-Daten handelt, entsprechen je 2 Bytes einem Sample.
    
    // Beispiel zur Demonstration: Wir geben alle 500 empfangenen Pakete 
    // eine Statusmeldung im Seriellen Monitor aus, um den Datenfluss zu beweisen.
    static int packet_counter = 0;
    packet_counter++;
    
    if (packet_counter >= 500) {
      Serial.printf("[INFO] Audio-Stream aktiv. %d Bytes eingelesen.\n", bytes_read);
      
      // Greife beispielhaft das erste 16-Bit Sample (Kombination aus zwei Bytes) ab:
      int16_t sample = (audio_buffer[1] << 8) | audio_buffer[0];
      Serial.printf("Aktueller Amplitude-Rohwert: %d\n", sample);
      
      packet_counter = 0;
    }
    
    // TIPP: Wenn du an den ESP32 einen Lautsprecher-DAC (z.B. MAX98357A) über I2S angebunden hast,
    // würdest du die Daten hier einfach per 'i2s_write' an den zweiten DAC-Port weiterreichen.

  }
}