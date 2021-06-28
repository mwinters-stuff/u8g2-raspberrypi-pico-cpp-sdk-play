
#ifndef U8G2FUNCTIONS_H
#define U8G2FUNCTIONS_H
#include "pico/stdlib.h"
#include "csrc/u8g2.h"


extern "C" uint8_t u8x8_byte_pico_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
extern "C" uint8_t u8x8_gpio_and_delay_template(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);


#endif