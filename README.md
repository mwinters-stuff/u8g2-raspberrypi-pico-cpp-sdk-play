Play code to make a MKS MINI12864 V2.0 work with a raspberry pi pico

This uses u8g2, and encoder-pio from pimoroni, using the cpp pico-sdk.

## u8g2

To get u8g2 to work, the CMakeLists.txt is patched to properly include it as a cmake project
Then the functions for the SPI interface were created and it works OK.

## Pin connections
See https://reprap.org/wiki/MKS_MINI_12864 for EXP1/EXP2 pinout.

|Pico|MINI12864|
|----|---------|
|16 GP12|BTN_EN1|
|17 GP13|BTN_EN2|
|19 GP14|BTN_ENC|
|20 GP15|DOGLCD_A0|
|30 RUN|KILL_PIN|
|25 MOSI|MOSI|
|24 SCK|SCK|
|22 CS0|DOGLCD_CS|
|21 MISO|MISO|
|V3.3|VCC|
|GND|GND|

Be sure to use 3.3volts instead of 5volts otherwise you might
damage your pico.


# what does it do?
* A hello snowman!
* Move the snowman left/right using the rotatry encoder
* Click the encoder to reset snowman to position 0
* Click the reset button to reset the Pico.
