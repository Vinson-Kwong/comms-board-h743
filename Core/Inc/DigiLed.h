/**
 * @file digitalled.h
 */

#ifndef DIGITALLED_H_
#define DIGITALLED_H_

/* includes */
#include <stdint.h>
#include "main.h"
// #include "spi.h"


/* defines */
#define 	LED_ROWS   		16
#define 	LED_COLS   		8
#define 	LED_FRAME_SIZE   		(LED_ROWS * LED_COLS)		///< \define number of LEDs in the chain

#define 	FALSE 					0		// false value
#define 	TRUE 					1		// true value
#define 	OUT_OF_RANGE			1		// chosen LED does not exist
#define 	RANGE_OK 				0		// chosen LED exist
#define 	LED_START_FRAME_SIZE 	4		// 0x00, 0x00, 0x00, 0x00
#define 	LED_END_FRAME_SIZE 		4 		// 0xFF, 0xFF, 0xFF, 0xFF




/// \class digitalled digitalled.h <digitalled.h>
/// \brief Manipulate a string of RGB LEds using SPI.
///
/// Supported are:
///	- WS2801
/// - APA102 (tested)
/// - APA102c
///
/// The class communicates with a object of the class type myspi that will interact with the API interface.







/* functions */
void DigiLed_init(SPI_HandleTypeDef *hspi);
void DigiLed_setColor(uint8_t led, uint8_t red, uint8_t green, uint8_t blue);
void DigiLed_setAllColor(uint8_t red, uint8_t green, uint8_t blue);
void DigiLed_setRGB(uint8_t led, uint32_t rgb);
void DigiLed_setAllRGB(uint32_t rgb);
void DigiLed_testRGB(uint32_t rgb);
void DigiLed_setLedIllumination(uint8_t led, uint8_t illumination);
void DigiLed_setAllIllumination(uint8_t illumination);
void DigiLed_setLedOff(uint8_t led);
void DigiLed_setAllRGBoff();
void DigiLed_setLedOn(uint8_t led);
void DigiLed_update(uint8_t forceUpdate);
uint8_t DigiLed_getFrameSize(void);
uint8_t DigiLed_TestPosition(uint8_t led);
void global_brightness_effect(uint32_t color, uint16_t duration_ms);





#endif /* DIGITALLED_H_ */
