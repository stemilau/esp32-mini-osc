#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define INTRO_DELAY 3500

#define OLED_RESET -1
#define OLED_SDA_1 19 // GPIO 22
#define OLED_SCL_1 18 // GPIO 23
#define OLED_SDA_2 17 // GPIO 19
#define OLED_SCL_2 16 // GPIO 18

#define BUTTON_OK 33
#define BUTTON_BACK 32
#define BUTTON_UP 35
#define BUTTON_DOWN 34

#define BUFF_SIZE 50000

enum Option {
  Autoset,
  Cx, //Cx
  Cy, //Cy
  VOffset, //VOffset
  TOffset,
  Mode, // AUTO -> AUTO:ANALOG/AUTO:DIGITAL;ANALOG;DIGITAL
  Filter,
  Single, // Single Trigger ?
  Acq, //Acquistion -> STOPPED/RUNNING
  Scale,
  Input,
  Help,
  NoOpt
};

enum MenuPages {
  PageOne,
  PageTwo,
  NoPage
};

TaskHandle_t task_menu;
TaskHandle_t task_adc;

uint16_t i2s_buff[BUFF_SIZE];
float RATE = 1000; //in ksps --> 1000 = 1Msps

int8_t volts_index = 0;
int8_t tscale_index = 0;

float v_div = 825;  //
float s_div = 10;   //
float offset = 0;   //
float toffset = 0;   //
uint8_t current_filter = 1; //

uint8_t opt = Autoset; 
uint8_t previous_opt = NoOpt; 
uint8_t menu_page = PageOne; 
uint8_t previous_menu_page = NoPage;

bool info = true;
bool set_value =false;
bool auto_set = false;
bool stop = false;

bool new_data = false;  //
bool menu_action = true;   //
bool updating_screen = false;   //
bool single_trigger = false;
bool digital_data = false;
uint8_t digital_wave_option = 0; //0-auto | 1-analog | 2-digital data (SERIAL/SPI/I2C/etc)

TwoWire I2C_OLED_1 = TwoWire(0);
TwoWire I2C_OLED_2 = TwoWire(1);

Adafruit_SH1106G OLED_Display_1(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED_1, OLED_RESET);
Adafruit_SH1106G OLED_Display_2(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED_2, OLED_RESET);

int btnok,btnup,btndown,btnback;  //

void IRAM_ATTR button_ok_isr(){
  btnok = 1;
}
void IRAM_ATTR button_back_isr(){
  btnback = 1;
}
void IRAM_ATTR button_up_isr(){
  btnup = 1;
}
void IRAM_ATTR button_down_isr(){
  btndown = 1;
}

void setup() {
  Serial.begin(115200);

  // configure_i2s(1000000);
  
  setup_oled_menu();
  setup_oled_wave();

  vTaskDelay(pdMS_TO_TICKS(INTRO_DELAY));
  OLED_Display_1.clearDisplay();
  OLED_Display_1.display();
  OLED_Display_2.clearDisplay(); // clears internal ram
  OLED_Display_2.display(); // loads info from ram into display controller ram

  write_wave_grid();

  switch (digital_wave_option) {
      case 0:
        write_menu_1_screen(v_div, s_div, offset, toffset, "AUTO");
        break;
      case 1:
        write_menu_1_screen(v_div, s_div, offset, toffset, "ANALOG");
        break;
      case 2:
        write_menu_1_screen(v_div, s_div, offset, toffset, "DIGITAL");
        break;
      default:
        write_menu_1_screen(v_div, s_div, offset, toffset, "AUTO");
        break;
    }

  

  pinMode(BUTTON_OK, INPUT);
  pinMode(BUTTON_BACK, INPUT);
  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_DOWN, INPUT);

  attachInterrupt(BUTTON_OK, button_ok_isr, RISING);
  attachInterrupt(BUTTON_BACK, button_back_isr, RISING);
  attachInterrupt(BUTTON_UP, button_up_isr, RISING);
  attachInterrupt(BUTTON_DOWN, button_down_isr, RISING);


  // characterize_adc();

  // #ifdef DEBUG_BUF
  //   debug_buffer();
  // #endif

  // xTaskCreatePinnedToCore(core0_task,
  //                         "menu_handle",
  //                         10000,  /* Stack size in words */
  //                         NULL,  /* Task input parameter */
  //                         0,  /* Priority of the task */
  //                         &task_menu,  /* Task handle. */
  //                         0); /* Core where the task should run */

  // xTaskCreatePinnedToCore(
  //   core1_task,
  //   "adc_handle",
  //   10000,  /* Stack size in words */
  //   NULL,  /* Task input parameter */
  //   3,  /* Priority of the task */
  //   &task_adc,  /* Task handle. */
  //   1); /* Core where the task should run */
}

void loop() {

}