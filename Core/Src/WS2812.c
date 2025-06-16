#include "WS2812.h"
#include "stdint.h"

/* variables */
uint8_t WS2812SpiSendFrame[3 * WS2812_LED_FRAME_SIZE];
uint8_t WS2812_frameModified; // when frame is changed the stimuli is set high
SPI_HandleTypeDef *WS2812_spiHandler;

union {
    struct {                // LSB
        uint8_t RED    : 8; ///< RED led intensity from 0 (off) to 255 (max)
        uint8_t GREEN  : 8; ///< GREEN led intensity from 0 (off) to 255 (max)
        uint8_t BLUE   : 8; ///< BLUE led intensity from 0 (off) to 255 (max)
        uint8_t GLOBAL : 5; ///< Global intensity for all LEDs from 0 (off) to 32 (max)
        uint8_t INIT   : 3; ///< Initialize, the led configuration starts with '111'
                            // MSB
    } FieldsIn;             ///< All fields in a LED packet
    struct {                // LSB
        uint8_t RED   : 8;  ///< RED led intensity from 0 (off) to 255 (max)
        uint8_t GREEN : 8;  ///< GREEN led intensity from 0 (off) to 255 (max)
        uint8_t BLUE  : 8;  ///< BLUE led intensity from 0 (off) to 255 (max)
        uint8_t CMD   : 8;  ///< Global intensity for all LEDs && frame start with
                            // MSB
    } FieldsOut;
    uint32_t data;                    ///< RAW LED packet data
} _WS2812talLedframe[WS2812_LED_FRAME_SIZE]; ///< Frame of LED packets

/* functions */

/**
 *  @brief Initialize WS2812tal led class
 */
void WS2812_init(SPI_HandleTypeDef *hspi)
{
    WS2812_frameModified = WS2812_TRUE; // Initial set to true to force update after initialization of frame buffer

    WS2812_spiHandler = hspi; // SPI handler is given to library

    // TODO Auto-generated constructor stub

    for (int led = 0; led < WS2812_LED_FRAME_SIZE; led++) {
        _WS2812talLedframe[led].FieldsIn.INIT = 0x07;   // Set MSB first 3 bits to identify start of LED packet
        _WS2812talLedframe[led].FieldsIn.GLOBAL = 0x00; // Switch off LED global
        _WS2812talLedframe[led].FieldsIn.BLUE = 0x00;
        _WS2812talLedframe[led].FieldsIn.GREEN = 0x00;
        _WS2812talLedframe[led].FieldsIn.RED = 0x00;
    }
    WS2812_update(WS2812_FALSE); // Update frame buffer using the value of WS2812_frameModified as set in initialiser.
}

/**
 * @brief set color of a single led
 * Set the colors of a single led ad position 'led' using single colors
 * @param led position of the led in the string
 * @param blue intensity of the blue color from 0 to 255
 * @param green intensity of the green color from 0 to 255
 * @param red intensity of the red color from 0 to 255
 */
void WS2812_setColor(uint8_t led, uint8_t red, uint8_t green, uint8_t blue)
{
    if (WS2812_TestPosition(led) == WS2812_RANGE_OK) {
        _WS2812talLedframe[led].FieldsIn.INIT = 0x7;    // Set MSB first 3 bits to identify start of LED packet
        _WS2812talLedframe[led].FieldsIn.GLOBAL = 0x1F; // Set led at maximum illumination
        _WS2812talLedframe[led].FieldsIn.BLUE = blue;
        _WS2812talLedframe[led].FieldsIn.GREEN = green;
        _WS2812talLedframe[led].FieldsIn.RED = red;
    }
    WS2812_frameModified = WS2812_TRUE;
}

/**
 * @brief set color of all LEDs in a string
 * @param blue intensity of the blue color from 0 to 255
 * @param green intensity of the green color from 0 to 255
 * @param red intensity of the red color from 0 to 255
 */
void WS2812_setAllColor(uint8_t red, uint8_t green, uint8_t blue)
{
    for (int led = 0; led < WS2812_LED_FRAME_SIZE; led++) {
        WS2812_setColor(led, red, green, blue);
    }
}

/**
 * @brief set color of a single led
 * Set the colors of a single led ad position 'led' using RGB color scheme
 * RGB colors are 24 bits of a 32 bit word where the intensity of the colors red, green en blue are
 * expressed as hex values from 0 to 255 (0 - FF).
 * Colors can be set using defines from "APA102_colors.h"
 * @param led position of the led in the string
 * @param rgb color of led in RGB color scheme
 */
void WS2812_setRGB(uint8_t led, uint32_t rgb)
{
    _WS2812talLedframe[led].FieldsIn.INIT = 0x7;
    _WS2812talLedframe[led].FieldsIn.GLOBAL = 0x1F;
    _WS2812talLedframe[led].FieldsIn.BLUE = (uint8_t)(rgb);
    _WS2812talLedframe[led].FieldsIn.GREEN = (uint8_t)(rgb >> 8);
    _WS2812talLedframe[led].FieldsIn.RED = (uint8_t)(rgb >> 16);
    WS2812_frameModified = WS2812_TRUE;
}

void WS2812_setARGB(uint8_t led, uint8_t brightness, uint32_t rgb)
{
    _WS2812talLedframe[led].FieldsIn.INIT = 0x7;
    _WS2812talLedframe[led].FieldsIn.GLOBAL = brightness & 0x1F;
    // _WS2812talLedframe[led].FieldsIn.GLOBAL = brightness & 0x07;
    _WS2812talLedframe[led].FieldsIn.BLUE = (uint8_t)(rgb);
    _WS2812talLedframe[led].FieldsIn.GREEN = (uint8_t)(rgb >> 8);
    _WS2812talLedframe[led].FieldsIn.RED = (uint8_t)(rgb >> 16);
    // _WS2812talLedframe[led].data = 0xffff; 				///< RAW LED packet data
    WS2812_frameModified = WS2812_TRUE;
}

/**
 * @brief set color of a single led
 * Set the colors of a single led ad position 'led' using RGB color scheme
 * RGB colors are 24 bits of a 32 bit word where the intensity of the colors red, green and blue are
 * expressed as hex values from 0 to 255 (0 - FF).
 * Colors can be set using defines from "APA102_colors.h"
 * @param rgb color of led in RGB color scheme
 */
void WS2812_setAllRGB(uint32_t rgb)
{
    for (int led = 0; led < WS2812_LED_FRAME_SIZE; led++) {
        WS2812_setRGB(led, rgb);
    }
    WS2812_frameModified = WS2812_TRUE;
}

void WS2812_testRGB(uint32_t rgb)
{
    for (int led = 0; led < WS2812_LED_FRAME_SIZE; led++) {
        WS2812_setARGB(led, led, rgb);
    }
    WS2812_frameModified = WS2812_TRUE;
}

/**
 * @brief set illumination of a single LED
 * Illumination is a value from 0 to 31. 0 means no light, and 31 maximum illumination.
 * setting illumination can interfere with individual RGB settings
 * @param led position of the led in the string
 * @param intensity of illumination
 */
void WS2812_setLedIllumination(uint8_t led, uint8_t illumination)
{
    if (WS2812_TestPosition(led) == WS2812_RANGE_OK) {
        _WS2812talLedframe[led].FieldsIn.GLOBAL = illumination;
    }
    WS2812_frameModified = WS2812_TRUE;
}

/**
 * @brief set illumination of a all LEDs in the frame
 * Illumination is a value from 0 to 31. 0 means no light, and 31 maximum illumination.
 * setting illumination can interfere with individual RGB settings
 * @param intensity of illumination
 */
void WS2812_setAllIllumination(uint8_t illumination)
{
    for (int led = 0; led < WS2812_LED_FRAME_SIZE; led++) {
        _WS2812talLedframe[led].FieldsIn.GLOBAL = illumination;
    }
    WS2812_frameModified = WS2812_TRUE;
}

/**
 * @brief switch a single led off
 * @param led position of the led in the string to be switched off
 */
void WS2812_setLedOff(uint8_t led)
{
    if (WS2812_TestPosition(led) == WS2812_RANGE_OK) {
        _WS2812talLedframe[led].FieldsIn.GLOBAL = 0x00;
    }
    WS2812_frameModified = WS2812_TRUE;
}

void WS2812_setAllRGBoff()
{
    for (int led = 0; led < WS2812_LED_FRAME_SIZE; led++) {
        WS2812_setLedOff(led);
    }
}

/**
 * @brief switch a single led on
 * Using this function will preserve the active color settings for the led
 * @param led position of the led in the string to be switched on
 */
void WS2812_setLedOn(uint8_t led)
{
    if (WS2812_TestPosition(led) == WS2812_RANGE_OK) {
        _WS2812talLedframe[led].FieldsIn.GLOBAL = 0x1F;
    }
    WS2812_frameModified = WS2812_TRUE;
}

/**
 * @brief update led string
 * @param set true to force update leds and false to update only when frame is modified
 */
void WS2812_update(uint8_t forceUpdate)
{
    if (WS2812_frameModified | forceUpdate) {
        // add all LED packets of the frame
        uint32_t SpiDataPacket = 0;
        for (uint32_t led = 0; led < WS2812_LED_FRAME_SIZE; led++) {
            WS2812SpiSendFrame[SpiDataPacket + 0] =
                _WS2812talLedframe[led].FieldsOut.BLUE; // Add BLUE to SPI send frame
            WS2812SpiSendFrame[SpiDataPacket + 1] =
                _WS2812talLedframe[led].FieldsOut.GREEN; // Add GREEN to SPI send frame
            WS2812SpiSendFrame[SpiDataPacket + 2] =
                _WS2812talLedframe[led].FieldsOut.RED; // Add RED to SPI send frame

            SpiDataPacket = SpiDataPacket + 4;
        }

        // send spi frame with all led values
        HAL_SPI_Transmit(WS2812_spiHandler, WS2812SpiSendFrame, sizeof(WS2812SpiSendFrame), 10);
    }

    WS2812_frameModified = WS2812_FALSE; // reset frame modified identifier.
}

/**
 * @brief get LED frame size
 * @return LED frame size
 */
uint8_t WS2812_getFrameSize(void)
{
    return WS2812_LED_FRAME_SIZE;
}

/**
 * @brief Test led position is within range.
 * @param led led position
 * @return result of evaluation ad define.
 */
uint8_t WS2812_TestPosition(uint8_t led)
{
    uint8_t returnValue = WS2812_OUT_OF_RANGE;
    if (led < WS2812_LED_FRAME_SIZE) {
        returnValue = WS2812_RANGE_OK;
    }
    return returnValue;
}

// 效果1：全部LED亮度从小到大渐变
void WS2812_global_brightness_effect(uint32_t color)
{
    const int delay = 10;
    uint8_t brightness = 0;

    for (int led = 0; led < WS2812_LED_FRAME_SIZE; led++) {
        WS2812_setARGB(led, brightness, color);
    }
    WS2812_update(1);
    HAL_Delay(delay);
}
