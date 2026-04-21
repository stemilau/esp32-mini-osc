#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <driver/i2s.h>

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   4

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define SIGNAL_PIN 34

#define WIDTH 160
#define HEIGHT 80

#define ST77XX_DARKGREY  0x7BEF
#define ST77XX_DARKGREEN 0x03E0

#define SAMPLES WIDTH

uint16_t buffer[SAMPLES];

int triggerLevel = 2048;

// ---------------- GRID CONTROL (FIX #3)
bool gridDrawn = false;

// ---------------- SETUP
void setup() {
  Serial.begin(115200);

  analogReadResolution(12);

  tft.initR(INITR_MINI160x80_PLUGIN);
  tft.setRotation(1);

  setupI2S();          // FIX #1
  drawGrid();          // FIX #3 (draw once)
}

// ---------------- MAIN LOOP
void loop() {

  waitForTrigger();    // FIX #2
  captureSamples();    // FIX #1
  drawWaveform();      // FIX #4 + #5
}

// =======================================================
// FIX #1 — I2S ADC (stable high-speed sampling)
// =======================================================
void setupI2S() {

  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = 100000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false
  };

  i2s_driver_install(I2S_NUM_0, &config, 0, NULL);
  i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_6); // GPIO34
  i2s_adc_enable(I2S_NUM_0);
}

// =======================================================
// FIX #1 — capture samples (I2S DMA)
// =======================================================
void captureSamples() {

  size_t bytesRead;

  i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);

  int count = bytesRead / 2;
  if (count > WIDTH) count = WIDTH;

  // FIX #5 — light smoothing (reduce noise)
  for (int i = 1; i < count; i++) {
    buffer[i] = (buffer[i] + buffer[i - 1]) / 2;
  }
}

// =======================================================
// FIX #2 — stable trigger (edge detection)
// =======================================================
void waitForTrigger() {

  int prev = analogRead(SIGNAL_PIN);

  while (true) {
    int current = analogRead(SIGNAL_PIN);

    if (prev < triggerLevel && current >= triggerLevel) {
      break;
    }

    prev = current;
  }
}

// =======================================================
// FIX #3 — grid drawn ONCE (no flicker)
// =======================================================
void drawGrid() {

  if (gridDrawn) return;

  tft.fillScreen(ST77XX_BLACK);

  for (int x = 20; x < WIDTH; x += 20)
    tft.drawFastVLine(x, 0, HEIGHT, ST77XX_DARKGREY);

  for (int y = 20; y < HEIGHT; y += 20)
    tft.drawFastHLine(0, y, WIDTH, ST77XX_DARKGREY);

  gridDrawn = true;
}

// =======================================================
// FIX #4 + #5 — smooth waveform drawing
// =======================================================
void drawWaveform() {

  int prevY = map(buffer[0], 0, 4095, HEIGHT - 1, 0);

  for (int x = 1; x < WIDTH; x++) {

    // FIX #5 — extra smoothing
    int value = (buffer[x] + buffer[x - 1]) / 2;

    int y = map(value, 0, 4095, HEIGHT - 1, 0);

    // main trace
    tft.drawLine(x - 1, prevY, x, y, ST77XX_GREEN);

    // phosphor glow
    tft.drawPixel(x, y + 1, ST77XX_DARKGREEN);
    tft.drawPixel(x, y - 1, ST77XX_DARKGREEN);

    prevY = y;
  }
}
