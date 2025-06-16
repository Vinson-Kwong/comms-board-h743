#ifndef __WS2818_H__
#define __WS2818_H__

#include <stdint.h>
#include "main.h"

#define WS2812_LED_ROWS       8
#define WS2812_LED_COLS       8
#define WS2812_LED_FRAME_SIZE (WS2812_LED_ROWS * WS2812_LED_COLS) ///< \define number of LEDs in the chain

#define WS2812_FALSE                0 // false value
#define WS2812_TRUE                 1 // true values
#define WS2812_OUT_OF_RANGE         1 // chosen LED does not exist
#define WS2812_RANGE_OK             0 // chosen LED exist

/// \class digitalled digitalled.h <digitalled.h>
/// \brief Manipulate a string of RGB LEds using SPI.
/// The class communicates with a object of the class type myspi that will interact with the API interface.

void WS2812_init(SPI_HandleTypeDef *hspi);
void WS2812_setColor(uint8_t led, uint8_t red, uint8_t green, uint8_t blue);
void WS2812_setAllColor(uint8_t red, uint8_t green, uint8_t blue);
void WS2812_setRGB(uint8_t led, uint32_t rgb);
void WS2812_setAllRGB(uint32_t rgb);
void WS2812_testRGB(uint32_t rgb);
void WS2812_setLedIllumination(uint8_t led, uint8_t illumination);
void WS2812_setAllIllumination(uint8_t illumination);
void WS2812_setLedOff(uint8_t led);
void WS2812_setAllRGBoff();
void WS2812_setLedOn(uint8_t led);
void WS2812_update(uint8_t forceUpdate);
uint8_t WS2812_getFrameSize(void);
uint8_t WS2812_TestPosition(uint8_t led);
void WS2812_global_brightness_effect(uint32_t color);

#endif /* __WS2818_H__ */
