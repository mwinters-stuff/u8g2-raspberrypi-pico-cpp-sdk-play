#include <iomanip>
#include <sstream>
#include <stdio.h>

#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/time.h"

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

Encoder encoder(pio0, ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_C,
                COUNTS_PER_REVOLUTION, COUNT_MICROSTEPS, FREQ_DIVIDER);


void drawDisplay(uint32_t position) {
  if(position > 100){
    return;
  }
  u8g2_ClearBuffer(&u8g2);
  u8g2_ClearDisplay(&u8g2);
  u8g2_SetFont(&u8g2, u8g2_font_6x13_tr); // choose a suitable font
  u8g2_DrawStr(&u8g2, 10, 10,
               "Hello Snowman!"); // write something to the internal memory

  std::stringstream sstream;
  sstream << "Count: " << position;
  u8g2_DrawStr(&u8g2, 10, 50,
               sstream.str().c_str()); // write something to the internal memory

  u8g2_SetDrawColor(&u8g2, 1);

  u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);
  u8g2_DrawGlyph(&u8g2, 10 + position, 30, 0x2603);
  u8g2_DrawFrame(&u8g2, 0, 0, u8g2_GetDisplayWidth(&u8g2),
                 u8g2_GetDisplayHeight(&u8g2));


  // u8g2_DrawBox(&u8g2, 30,30,50,50);
  u8g2_UpdateDisplay(&u8g2);
  u8g2_SendBuffer(&u8g2);
}

void display_core() {
  u8g2_Setup_uc1701_mini12864_f(&u8g2, U8G2_R0, u8x8_byte_pico_hw_spi,
                                u8x8_gpio_and_delay_template);
  u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in
  u8g2_SetPowerSave(&u8g2, 0); // wake up display
  u8g2_SetContrast(&u8g2, 200);

  drawDisplay(0);

  while (1) {
    uint32_t delay = multicore_fifo_pop_blocking();
    drawDisplay(delay);
  }
}

uint32_t flashDelay = 0;
uint32_t lastPosition = 0;
bool repeating_timer_callback(struct repeating_timer *t) {
  Capture capture = encoder.perform_capture();
  {
    std::stringstream sstream;
    sstream << "Count: " << capture.get_count() << "\n";
    printf(sstream.str().c_str());
  }

  {
    std::stringstream sstream;
    sstream << "Freq: " << std::fixed << std::setprecision(1)
            << capture.get_frequency() << "hz"
            << "\n";
    printf(sstream.str().c_str());
  }

  {
    std::stringstream sstream;
    sstream << "RPM:" << std::fixed << std::setprecision(1)
            << capture.get_revolutions_per_minute() << "\n";
    printf(sstream.str().c_str());
  }
  if (lastPosition != capture.get_count()) {
    multicore_fifo_push_blocking(capture.get_count());
    lastPosition = capture.get_count();
    flashDelay = 100 + (lastPosition *10);
  }

  printf("Button %s\n",   gpio_get(ENCODER_SWITCH_PIN) ? "true":  "false" );

  return true;
}

void gpio_callback(uint gpio, uint32_t events) {
    encoder.zero_count();
    multicore_fifo_push_blocking(0);
}

int main() {
  stdio_init_all();

  puts("Hello, world!");

  const uint LED_PIN = PICO_DEFAULT_LED_PIN;
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  multicore_launch_core1(display_core);

  if (ENCODER_SWITCH_PIN != Encoder::PIN_UNUSED) {
    gpio_init(ENCODER_SWITCH_PIN);
    gpio_set_dir(ENCODER_SWITCH_PIN, GPIO_IN);
    gpio_pull_up(ENCODER_SWITCH_PIN);
  }
  encoder.init();


  struct repeating_timer timer;
  add_repeating_timer_ms(-500, repeating_timer_callback, NULL, &timer);

  gpio_set_irq_enabled_with_callback(ENCODER_SWITCH_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
  auto nextTime = make_timeout_time_ms(flashDelay);
  while (true) {
    if (absolute_time_diff_us(nextTime, get_absolute_time()) >= 0) {
      gpio_put(LED_PIN, !gpio_get_out_level(LED_PIN));
      nextTime = make_timeout_time_ms(flashDelay);
    }
  }

  return 0;
}
