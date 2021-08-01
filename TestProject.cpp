#include <iomanip>
#include <sstream>
#include <stdio.h>

#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/util/queue.h"

#include "csrc/u8g2.h"
#include "encoder.hpp"

#include "u8g2functions.h"

using namespace pimoroni;

static const uint8_t ENCODER_PIN_A = 12;
static const uint8_t ENCODER_PIN_B = 13;
static const uint8_t ENCODER_PIN_C = Encoder::PIN_UNUSED;
static const uint8_t ENCODER_SWITCH_PIN = 14;

static constexpr float COUNTS_PER_REVOLUTION =
    24; // 24 is for rotary encoders. For motor magnetic encoders uses
        // 12 times the gear ratio (e.g. 12 * 20 with a 20:1 ratio motor
static const bool COUNT_MICROSTEPS =
    false; // Set to true for motor magnetic encoders

static const uint16_t FREQ_DIVIDER =
    1; // Increase this to deal with switch bounce. 250 Gives a 1ms debounce

u8g2_t u8g2;
bool spiInit = false;
queue_t queue;

Encoder encoder(pio0, ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_C,
                COUNTS_PER_REVOLUTION, COUNT_MICROSTEPS, FREQ_DIVIDER);

uint32_t flashDelay = 0;
int32_t lastCount = 0;
int32_t position = 0;

int8_t snowmanTop;
int8_t snowmanWidth;
int8_t snowmanHeight;
int8_t fontHeight;
uint16_t displayWidth;
uint16_t displayHeight;

void drawDisplay(uint32_t position) {
  char msg[15] = "Hello Snowman!";
  if (position == 0) {
    u8g2_ClearBuffer(&u8g2);
    u8g2_ClearDisplay(&u8g2);
    u8g2_SetMaxClipWindow(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_6x13_tr); // choose a suitable font

    u8g2_DrawStr(&u8g2, (displayWidth - u8g2_GetStrWidth(&u8g2, msg)) / 2,
                 u8g2_GetMaxCharHeight(&u8g2) + 2,
                 msg); // write something to the internal memory

    snprintf(msg, 15, "Count: %d", position);
    u8g2_DrawStr(&u8g2, (displayWidth - u8g2_GetStrWidth(&u8g2, msg)) / 2,
                 displayHeight - 2, msg); // write something to the internal memory

    u8g2_DrawFrame(&u8g2, 0, 0, displayWidth, displayHeight);

    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_UpdateDisplay(&u8g2);
  }
  u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);

  
  u8g2_SetClipWindow(&u8g2, 1, snowmanTop - snowmanHeight, displayWidth - 1, snowmanTop);
  u8g2_SetDrawColor(&u8g2, 0);
  u8g2_DrawBox(&u8g2, 1, snowmanTop - snowmanHeight, displayWidth - 1, snowmanTop);
  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawGlyph(&u8g2, 1 + position, snowmanTop, 0x2603);


  u8g2_SetFont(&u8g2, u8g2_font_6x13_tr); // choose a suitable font
  
  u8g2_SetClipWindow(&u8g2, 1, displayHeight - 2 - fontHeight, displayWidth - 1, displayHeight - 2);

  u8g2_SetDrawColor(&u8g2, 0);
  u8g2_DrawBox(&u8g2, 1, displayHeight - 2 - fontHeight, displayWidth - 1, displayHeight - 2);
  u8g2_SetDrawColor(&u8g2, 1);

  snprintf(msg, 15, "Count: %d", position);
  u8g2_DrawStr(&u8g2, (displayWidth - u8g2_GetStrWidth(&u8g2, msg)) / 2,
               displayHeight - 2, msg); // write something to the internal memory

  u8g2_UpdateDisplay(&u8g2);
}


void display_core() {
  u8g2_Setup_uc1701_mini12864_f(&u8g2, U8G2_R0, u8x8_byte_pico_hw_spi,
                                u8x8_gpio_and_delay_template);
  u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in
  u8g2_SetPowerSave(&u8g2, 0); // wake up display
  u8g2_SetContrast(&u8g2, 200);

  displayHeight = u8g2_GetDisplayHeight(&u8g2);
  displayWidth = u8g2_GetDisplayWidth(&u8g2);

  u8g2_SetFont(&u8g2, u8g2_font_6x13_tr); // choose a suitable font
  fontHeight = u8g2_GetMaxCharHeight(&u8g2);
  
  u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);
  snowmanWidth = u8g2_GetMaxCharWidth(&u8g2);
  snowmanHeight = u8g2_GetMaxCharHeight(&u8g2);
  snowmanTop = ((displayHeight - snowmanHeight) / 2) + snowmanHeight;


  drawDisplay(0);

  int32_t count;
  while (1) {
    queue_remove_blocking(&queue, &count);
    if(count == INT32_MIN){
      lastCount = 0;
      position = 0;
      drawDisplay(position);
    }else if(count != lastCount){
      position += count - lastCount;
      if (position < 0) {
        position = 0;
      }
      if(position > displayWidth - 2 - snowmanWidth){
        position = displayWidth - 2 - snowmanWidth;
      }
      drawDisplay(position);
      lastCount = count;
    
    }
  }
}


bool repeating_timer_callback(struct repeating_timer *t) {
  int32_t count = encoder.get_count();
  queue_try_add(&queue, &count);
  flashDelay = 100 + (count * 10);
  return true;
}

void gpio_callback(uint gpio, uint32_t events) {
  encoder.zero_count();
  int32_t value = INT32_MIN;
  queue_try_add(&queue, &value);
  // multicore_fifo_push_blocking(0);
}

int main() {
  stdio_init_all();

  puts("Hello, world!");

  const uint LED_PIN = PICO_DEFAULT_LED_PIN;
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  queue_init_with_spinlock(&queue, sizeof(int32_t), 10, 1);

  multicore_launch_core1(display_core);

  if (ENCODER_SWITCH_PIN != Encoder::PIN_UNUSED) {
    gpio_init(ENCODER_SWITCH_PIN);
    gpio_set_dir(ENCODER_SWITCH_PIN, GPIO_IN);
    gpio_pull_up(ENCODER_SWITCH_PIN);
  }
  encoder.init();

  struct repeating_timer timer;
  add_repeating_timer_ms(-500, repeating_timer_callback, NULL, &timer);

  gpio_set_irq_enabled_with_callback(ENCODER_SWITCH_PIN, GPIO_IRQ_EDGE_FALL,
                                     true, &gpio_callback);
  auto nextTime = make_timeout_time_ms(flashDelay);
  while (true) {
    if (absolute_time_diff_us(nextTime, get_absolute_time()) >= 0) {
      gpio_put(LED_PIN, !gpio_get_out_level(LED_PIN));
      nextTime = make_timeout_time_ms(flashDelay);
    }
  }

  return 0;
}
