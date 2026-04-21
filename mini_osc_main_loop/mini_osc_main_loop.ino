#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define SIGNAL_PIN 34

#define WIDTH 160
#define HEIGHT 80

#define ST77XX_DARKGREY  0x7BEF
#define ST77XX_DARKGREEN 0x03E0

#define SAMPLES WIDTH

uint16_t buffer[SAMPLES];

int triggerLevel = 2048; // mid-level trigger

//----------------------------------

void setup() {
  analogReadResolution(12);

  tft.initR(INITR_MINI160x80_PLUGIN);
  tft.setRotation(1);

  tft.fillScreen(ST77XX_BLACK);
  drawGrid();
}

//----------------------------------

void loop() {
  captureSamples();
  drawWaveform();
}

//----------------------------------
// GRID (oscilloscope style)

void drawGrid() {
  tft.fillScreen(ST77XX_BLACK);

  for (int x =20; x < WIDTH; x += 20)
    tft.drawFastVLine(x, 0, HEIGHT, ST77XX_DARKGREY);

  for (int y = 20; y < HEIGHT; y += 20)
    tft.drawFastHLine(0, y, WIDTH, ST77XX_DARKGREY);
}

//----------------------------------
// CAPTURE (with trigger like real scope)

void captureSamples() {

  // Wait for rising edge trigger
  while (analogRead(SIGNAL_PIN) < triggerLevel);
  while (analogRead(SIGNAL_PIN) >= triggerLevel);

  for (int i = 0; i < SAMPLES; i++) {
    buffer[i] = analogRead(SIGNAL_PIN);

    // Faster than Arduino Uno — tweak this
    delayMicroseconds(20);
  }
}

//----------------------------------
// DRAW waveform (smooth + glow)

void drawWaveform() {

  drawGrid(); // clear previous frame

  int prevY = map(buffer[0], 0, 4095, HEIGHT - 1, 0);

  for (int x = 1; x < WIDTH; x++) {

    int y = map(buffer[x], 0, 4095, HEIGHT - 1, 0);

    // Main bright trace
    tft.drawLine(x - 1, prevY, x, y, ST77XX_GREEN);

    // Glow effect (phosphor look)
    tft.drawPixel(x, y + 1, ST77XX_DARKGREEN);
    tft.drawPixel(x, y - 1, ST77XX_DARKGREEN);

    prevY = y;
  }
}
