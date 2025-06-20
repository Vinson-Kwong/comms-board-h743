/**
 * @brief		This library enables the use of the popular APA102 addressable
 * 				RGB LEDs with the familiar STM32 HAL library.
 * @file		digitalled.c
 * @date		13.07.2019
 * @version		0.1.0
 * @author		Hans Achterbahn
 * @url			https://github.com/HansAchterbahn/APA102-on-STM32
 * @license		CC-BY-SA 4.0
 *
 *
 * @forkAuthor		Remko Welling
 * @forkE-Mail		remko@rfsee.nl
 * @forkUrl			https://github.com/pe1mew/APA102-on-STM32
 * @forkLicense		CC-BY-SA 4.0
 *
 *
 * @settings
 *
 * 		SPI Settings
 * 		------------
 * 		hspi1.Instance = SPI1;
 * 		hspi1.Init.Mode = SPI_MODE_MASTER;
 * 		hspi1.Init.Direction = SPI_DIRECTION_1LINE;
 * 		hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
 * 		hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
 * 		hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
 * 		hspi1.Init.NSS = SPI_NSS_SOFT;
 * 		hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
 * 		hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
 * 		hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
 * 		hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
 *	 	hspi1.Init.CRCPolynomial = 7;
 * 		hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
 * 		hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
 *
 */

/* includes */
#include "DigiLed.h"
#include "stdint.h"


/* variables */
uint8_t SpiSendFrame[LED_START_FRAME_SIZE + 4 * LED_FRAME_SIZE + LED_END_FRAME_SIZE];
uint8_t _frameModified; 		// when frame is changed the stimuli is set high
SPI_HandleTypeDef *_spiHandler;

union
{
	struct
	{					// LSB
		uint8_t RED    : 8;	///< RED led intensity from 0 (off) to 255 (max)
		uint8_t GREEN  : 8; 	///< GREEN led intensity from 0 (off) to 255 (max)
		uint8_t BLUE   : 8; 	///< BLUE led intensity from 0 (off) to 255 (max)
		uint8_t GLOBAL : 5; 	///< Global intensity for all LEDs from 0 (off) to 32 (max)
		uint8_t INIT   : 3; 	///< Initialize, the led configuration starts with '111'
						// MSB
	} FieldsIn;					///< All fields in a LED packet
	struct
	{					// LSB
		uint8_t RED    : 8;		///< RED led intensity from 0 (off) to 255 (max)
		uint8_t GREEN  : 8; 	///< GREEN led intensity from 0 (off) to 255 (max)
		uint8_t BLUE   : 8; 	///< BLUE led intensity from 0 (off) to 255 (max)
		uint8_t CMD	   : 8; 	///< Global intensity for all LEDs && frame start with
						// MSB
			} FieldsOut;
	uint32_t data; 				///< RAW LED packet data
} _digitalLedframe[LED_FRAME_SIZE];	///< Frame of LED packets

/* functions */

/**
 *  @brief Initialize digital led class
 */
void DigiLed_init(SPI_HandleTypeDef *hspi)
{
	_frameModified = TRUE; 		// Initial set to true to force update after initialization of frame buffer

	_spiHandler = hspi;			// SPI handler is given to library

	// TODO Auto-generated constructor stub

	for (int led = 0; led < LED_FRAME_SIZE; led++)
	{
		_digitalLedframe[led].FieldsIn.INIT = 0x07; // Set MSB first 3 bits to identify start of LED packet
		_digitalLedframe[led].FieldsIn.GLOBAL = 0x00; // Switch off LED global
		_digitalLedframe[led].FieldsIn.BLUE = 0x00;
		_digitalLedframe[led].FieldsIn.GREEN = 0x00;
		_digitalLedframe[led].FieldsIn.RED = 0x00;
	}
	DigiLed_update(FALSE); // Update frame buffer using the value of _frameModified as set in initialiser.
}


/**
 * @brief set color of a single led
 * Set the colors of a single led ad position 'led' using single colors
 * @param led position of the led in the string
 * @param blue intensity of the blue color from 0 to 255
 * @param green intensity of the green color from 0 to 255
 * @param red intensity of the red color from 0 to 255
 */
void DigiLed_setColor(uint8_t led, uint8_t red, uint8_t green, uint8_t blue)
{
	if (DigiLed_TestPosition(led) == RANGE_OK)
	{
		_digitalLedframe[led].FieldsIn.INIT = 0x7; // Set MSB first 3 bits to identify start of LED packet
		_digitalLedframe[led].FieldsIn.GLOBAL = 0x1F; // Set led at maximum illumination
		_digitalLedframe[led].FieldsIn.BLUE = blue;
		_digitalLedframe[led].FieldsIn.GREEN = green;
		_digitalLedframe[led].FieldsIn.RED = red;
	}
	_frameModified = TRUE;
}


/**
 * @brief set color of all LEDs in a string
 * @param blue intensity of the blue color from 0 to 255
 * @param green intensity of the green color from 0 to 255
 * @param red intensity of the red color from 0 to 255
 */
void DigiLed_setAllColor(uint8_t red, uint8_t green, uint8_t blue)
{
	for (int led = 0; led < LED_FRAME_SIZE; led++)
	{
		DigiLed_setColor(led, red, green, blue);
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
void DigiLed_setRGB(uint8_t led, uint32_t rgb)
{
	_digitalLedframe[led].FieldsIn.INIT = 0x7;
	_digitalLedframe[led].FieldsIn.GLOBAL = 0x1F;
	_digitalLedframe[led].FieldsIn.BLUE = (uint8_t)(rgb);
	_digitalLedframe[led].FieldsIn.GREEN = (uint8_t)(rgb >> 8);
	_digitalLedframe[led].FieldsIn.RED = (uint8_t)(rgb >> 16);
	_frameModified = TRUE;
}

void DigiLed_setARGB(uint8_t led, uint8_t brightness, uint32_t rgb)
{
	_digitalLedframe[led].FieldsIn.INIT = 0x7;
	_digitalLedframe[led].FieldsIn.GLOBAL = brightness & 0x1F;
	// _digitalLedframe[led].FieldsIn.GLOBAL = brightness & 0x07;
	_digitalLedframe[led].FieldsIn.BLUE = (uint8_t)(rgb);
	_digitalLedframe[led].FieldsIn.GREEN = (uint8_t)(rgb >> 8);
	_digitalLedframe[led].FieldsIn.RED = (uint8_t)(rgb >> 16);
	// _digitalLedframe[led].data = 0xffff; 				///< RAW LED packet data
	_frameModified = TRUE;
}

/**
 * @brief set color of a single led
 * Set the colors of a single led ad position 'led' using RGB color scheme
 * RGB colors are 24 bits of a 32 bit word where the intensity of the colors red, green and blue are
 * expressed as hex values from 0 to 255 (0 - FF).
 * Colors can be set using defines from "APA102_colors.h"
 * @param rgb color of led in RGB color scheme
 */
void DigiLed_setAllRGB(uint32_t rgb)
{
	for (int led = 0; led < LED_FRAME_SIZE; led++)
	{
		DigiLed_setRGB(led, rgb);
	}
	_frameModified = TRUE;
}

void DigiLed_testRGB(uint32_t rgb)
{
	for (int led = 0; led < LED_FRAME_SIZE; led++)
	{
		DigiLed_setARGB(led, led, rgb);
	}
	_frameModified = TRUE;
}


/**
 * @brief set illumination of a single LED
 * Illumination is a value from 0 to 31. 0 means no light, and 31 maximum illumination.
 * setting illumination can interfere with individual RGB settings
 * @param led position of the led in the string
 * @param intensity of illumination
 */
void DigiLed_setLedIllumination(uint8_t led, uint8_t illumination)
{
	if (DigiLed_TestPosition(led) == RANGE_OK)
	{
		_digitalLedframe[led].FieldsIn.GLOBAL = illumination;
	}
	_frameModified = TRUE;
}


/**
 * @brief set illumination of a all LEDs in the frame
 * Illumination is a value from 0 to 31. 0 means no light, and 31 maximum illumination.
 * setting illumination can interfere with individual RGB settings
 * @param intensity of illumination
 */
void DigiLed_setAllIllumination(uint8_t illumination)
{
	for (int led = 0; led < LED_FRAME_SIZE; led++)
	{
		_digitalLedframe[led].FieldsIn.GLOBAL = illumination;
	}
	_frameModified = TRUE;
}


/**
 * @brief switch a single led off
 * @param led position of the led in the string to be switched off
 */
void DigiLed_setLedOff(uint8_t led)
{
	if (DigiLed_TestPosition(led) == RANGE_OK)
	{
		_digitalLedframe[led].FieldsIn.GLOBAL = 0x00;
	}
	_frameModified = TRUE;
}


void DigiLed_setAllRGBoff()
{
	for (int led = 0; led < LED_FRAME_SIZE; led++)
	{
		DigiLed_setLedOff(led);
	}
}

/**
 * @brief switch a single led on
 * Using this function will preserve the active color settings for the led
 * @param led position of the led in the string to be switched on
 */
void DigiLed_setLedOn(uint8_t led)
{
	if (DigiLed_TestPosition(led) == RANGE_OK)
	{
		_digitalLedframe[led].FieldsIn.GLOBAL = 0x1F;
	}
	_frameModified = TRUE;
}


/**
 * @brief update led string
 * @param set true to force update leds and false to update only when frame is modified
 */
void DigiLed_update(uint8_t forceUpdate)
{
	if(_frameModified | forceUpdate)
	{
		// add start of frame (0x00000000)
		for(int i = 0; i < LED_START_FRAME_SIZE; i++)
		{
			SpiSendFrame[i] = 0x00;
		}

		// add all LED packets of the frame
		uint32_t SpiDataPacket = 0;
		for (uint32_t led = 0; led < LED_FRAME_SIZE; led++)
		{
			SpiSendFrame[LED_START_FRAME_SIZE + SpiDataPacket + 0] = _digitalLedframe[led].FieldsOut.CMD;		// Add INIT and GLOBAL to SPI send frame
			SpiSendFrame[LED_START_FRAME_SIZE + SpiDataPacket + 1] = _digitalLedframe[led].FieldsOut.BLUE; 	// Add BLUE to SPI send frame
			SpiSendFrame[LED_START_FRAME_SIZE + SpiDataPacket + 2] = _digitalLedframe[led].FieldsOut.GREEN;	// Add GREEN to SPI send frame
			SpiSendFrame[LED_START_FRAME_SIZE + SpiDataPacket + 3] = _digitalLedframe[led].FieldsOut.RED;		// Add RED to SPI send frame

			SpiDataPacket = SpiDataPacket + 4;
		}

		// add end of frame (0xffffffff)
		for(int i = 0; i < 4; i++)
		{
			// SpiSendFrame[LED_START_FRAME_SIZE + 4*LED_FRAME_SIZE + i] = 0xFF;
			SpiSendFrame[LED_START_FRAME_SIZE + 4*LED_FRAME_SIZE + i] = 0x00;
		}

		// send spi frame with all led values
		HAL_SPI_Transmit(_spiHandler, SpiSendFrame, sizeof(SpiSendFrame), 10);
	}

	_frameModified = FALSE; // reset frame modified identifier.

}


/**
 * @brief get LED frame size
 * @return LED frame size
 */
uint8_t DigiLed_getFrameSize(void)
{
	return LED_FRAME_SIZE;
}


/**
 * @brief Test led position is within range.
 * @param led led position
 * @return result of evaluation ad define.
 */
uint8_t DigiLed_TestPosition(uint8_t led)
{
	uint8_t returnValue = OUT_OF_RANGE;
	if (led < LED_FRAME_SIZE)
	{
		returnValue = RANGE_OK;
	}
	return returnValue;
}

void row_scan_running_light(uint8_t brightness, uint32_t color, uint16_t delay_ms) {
    // 从左到右逐列扫描
    for(int col = 0; col < LED_COLS; col++) {
        // 关闭所有LED
        for(int i = 0; i < LED_FRAME_SIZE; i++) {
            DigiLed_setARGB(i, 0, 0);
        }
        // 点亮当前列的所有LED
        for(int row = 0; row < LED_ROWS; row++) {
            int led_num = row * LED_COLS + col;
            DigiLed_setARGB(led_num, brightness, color);
        }
        DigiLed_update(1);
        HAL_Delay(delay_ms);
    }
    
    // 从右到左逐列扫描
    for(int col = LED_COLS-1; col >= 0; col--) {
        // 关闭所有LED
        for(int i = 0; i < LED_FRAME_SIZE; i++) {
            DigiLed_setARGB(i, 0, 0);
        }
        // 点亮当前列的所有LED
        for(int row = 0; row < LED_ROWS; row++) {
            int led_num = row * LED_COLS + col;
            DigiLed_setARGB(led_num, brightness, color);
        }
        DigiLed_update(1);
        HAL_Delay(delay_ms);
    }
}

void spiral_running_light(uint8_t brightness, uint32_t color, uint16_t delay_ms)
{
    // 定义边界
    int top = 0, bottom = LED_ROWS - 1;
    int left = 0, right = LED_COLS - 1;

    while (top <= bottom && left <= right) {
        // 关闭所有LED
        for (int i = 0; i < LED_FRAME_SIZE; i++) {
            DigiLed_setARGB(i, 0, 0);
        }

        // 从左到右扫描顶部行
        for (int col = left; col <= right; col++) {
            int led_num = top * LED_COLS + col;
            DigiLed_setARGB(led_num, brightness, color);
        }
        DigiLed_update(1);
        HAL_Delay(delay_ms);
        top++;

        // 从上到下扫描右侧列
        for (int row = top; row <= bottom; row++) {
            int led_num = row * LED_COLS + right;
            DigiLed_setARGB(led_num, brightness, color);
        }
        DigiLed_update(1);
        HAL_Delay(delay_ms);
        right--;

        if (top <= bottom) {
            // 从右到左扫描底部行
            for (int col = right; col >= left; col--) {
                int led_num = bottom * LED_COLS + col;
                DigiLed_setARGB(led_num, brightness, color);
            }
            DigiLed_update(1);
            HAL_Delay(delay_ms);
            bottom--;
        }

        if (left <= right) {
            // 从下到上扫描左侧列
            for (int row = bottom; row >= top; row--) {
                int led_num = row * LED_COLS + left;
                DigiLed_setARGB(led_num, brightness, color);
            }
            DigiLed_update(1);
            HAL_Delay(delay_ms);
            left++;
        }
    }
}

// 平滑亮度变化函数（0-31范围）
uint8_t smooth_brightness(float progress) {
    // 使用正弦函数实现平滑变化 (0-π区间)
    float sin_value = sinf(progress * M_PI);
    return (uint8_t)(sin_value * 31);
}

// 效果2：亮度流水移动效果
void brightness_wave_effect(uint32_t color, uint16_t wave_speed_ms, uint8_t wave_width) {
    for(int pos = 0; pos < LED_FRAME_SIZE + wave_width; pos++) {
        // 设置所有LED亮度
        for(int led = 0; led < LED_FRAME_SIZE; led++) {
            // 计算当前LED在波形中的位置
            int wave_pos = led - pos;
            
            // 计算亮度值（三角形波）
            uint8_t brightness = 0;
            if(wave_pos >= 0 && wave_pos < wave_width) {
                // 上升沿
                if(wave_pos < wave_width/2) {
                    brightness = 31 * wave_pos * 2 / wave_width;
                } 
                // 下降沿
                else {
                    brightness = 31 * (wave_width - wave_pos) * 2 / wave_width;
                }
            }
            
            DigiLed_setARGB(led, brightness, color);
        }
        DigiLed_update(1);
        HAL_Delay(wave_speed_ms);
    }
}

// 效果1：全部LED亮度从小到大渐变
void global_brightness_effect(uint32_t color, uint16_t duration_ms) {
    const int steps = 50; // 渐变步数
    const int delay = duration_ms / steps;
    
    for(int i = 0; i <= steps; i++) {
        float progress = (float)i / steps;
        uint8_t brightness = smooth_brightness(progress);
        
        // 设置所有LED为当前亮度
        for(int led = 0; led < LED_FRAME_SIZE; led++) {
            DigiLed_setARGB(led, brightness, color);
        }
        DigiLed_update(1);
        HAL_Delay(delay);
    }
}
