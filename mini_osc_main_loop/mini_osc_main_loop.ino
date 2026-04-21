#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <driver/i2s.h>

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
uint16_t prevBuffer[SAMPLES];

int triggerLevel = 2048;

// persistence strength (higher = longer glow)
#define FADE_AMOUNT 20

bool gridDrawn = false;

// =======================================================
// SETUP
// =======================================================
void setup() {

  analogReadResolution(12);

  tft.initR(INITR_MINI160x80_PLUGIN);
  tft.setRotation(1);

  setupI2S();
  drawGrid();
}

// =======================================================
// LOOP
// =======================================================
void loop() {

  waitForTrigger();
  captureSamples();
  drawWaveform();
}

// =======================================================
// I2S ADC SETUP (HIGH SPEED)
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
// TRIGGER (stable edge detection)
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
// CAPTURE (I2S DMA)
// =======================================================
void captureSamples() {

  size_t bytesRead;

  i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);

  int count = bytesRead / 2;
  if (count > WIDTH) count = WIDTH;

  // smoothing (simple low-pass filter)
  for (int i = 1; i < count; i++) {
    buffer[i] = (buffer[i] + buffer[i - 1]) / 2;
  }
}

// =======================================================
// GRID (draw ONCE ONLY)
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
// PERSISTENCE FADE (phosphor effect)
// =======================================================
void fadeScreen() {

  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {

      uint16_t c = tft.readPixel(x, y);

      // fade green channel slightly
      if (c != ST77XX_BLACK) {
        tft.drawPixel(x, y, c & 0xF7DE); // reduce brightness
      }
    }
  }
}

// =======================================================
// WAVEFORM RENDER (REAL DSO STYLE)
// =======================================================
void drawWaveform() {

  fadeScreen(); // persistence effect ⭐

  for (int x = 1; x < WIDTH; x++) {

    int value = (buffer[x] + buffer[x - 1]) / 2;
    int y = map(value, 0, 4095, HEIGHT - 1, 0);

    int prevY = map(prevBuffer[x], 0, 4095, HEIGHT - 1, 0);

    // erase old trace slightly (NOT full clear)
    tft.drawPixel(x, prevY, ST77XX_BLACK);

    // draw new trace
    tft.drawPixel(x, y, ST77XX_GREEN);
    tft.drawPixel(x, y + 1, ST77XX_DARKGREEN);
    tft.drawPixel(x, y - 1, ST77XX_DARKGREEN);

    prevBuffer[x] = buffer[x];
  }
}
