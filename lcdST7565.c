/************************************************************************
 *  @file lcdST7565.c
 *  
 *  @brief
 *  
 *   Interface program for ST7656 Graphic LCD 128x64 pixels B/W
 *  ------------------------------------------------------------
 *  
 *  This program help to initialize, write and draw graphic on this
 *  LCD via a command line interface on Raspberry Pi.
 *  
 *  Adafruit White LED ST7565 LCD 
 *  Product: http://www.adafruit.com/products/250
 *  LCD Data-sheet: http://goo.gl/pZO0Ng 
 *  Most of the details are packed up with this program.
 *  
 *  This program uses the 'pigpio' library as the basis to perform the 
 *  interfacing functions. The LCD uses the SPI0 interface a few other
 *  GPIO controlled using this library to talk to the LCD.
 *  Here is web-page for the 'pigpio' library: http://goo.gl/zIaF9G
 *  Thanks to this library the Graphic LCD interface is very easy to
 *  implement.
 *  
 *  Font generation is done using the MikroElektronica GLCD Font Creator:
 *  http://www.mikroe.com/glcd-font-creator/
 *  
 *  The next steps would be integrate this driver into the Frame Buffer
 *  kernel driver inside Raspberry Pi. 
 *  
 *  @note
 *  Author: boseji <prog.ic@live.in>
 *  
 *  @license
 *  License:
 *  This work is licensed under a Creative Commons 
 *  Attribution-ShareAlike 4.0 International License.
 *  http://creativecommons.org/licenses/by-sa/4.0/
 *  
 ************************************************************************/


/************************************************************************/
/* Includes                                                             */
/************************************************************************/

#include <stdio.h>
#include <pigpio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

/************************************************************************/
/* Rpi-ST7565 LCD Connection with 40pin RpiB+ or 26pin RpiB             */
/************************************************************************/
   
/*  
    Rpi Connector      ST7565 LCD
    ------------------------------
    3.3v (Pin01)     - LCD Back A
    GND  (Pin09)     - LCD Back K
    GND  (Pin06)     - GND
    3.3V (Pin17)     - VCC

    GPIO10(SPI_MOSI) - SID
    GPIO11(SPI_CLK)  - SCLK
    GPIO24           - A0
    GPIO25           - nRST
    GPIO08(SPI_CE0N) - nCS   */
    
/************************************************************************/

/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

/* Global SPI bus handle */
static int gx_spihandle = -1;

/* Global position storage for the Cursor */
static uint16_t gw_row = 0;
static uint16_t gw_column = 0;

/* Adafruit White LED ST7565 LCD 
   128 x 64 pixels
   http://www.adafruit.com/products/250
   - This needs to be enabled if using the Adafruit version
    of the LCD. The problem is with the weird page memory locations.
    Also the Number of columns is one on excess of 128.
   
   - Comment this out when using the Normal LCD version which does 
   not have the changeover in the page memory and the additional line.
    
   LCD Datasheet: http://goo.gl/pZO0Ng   
    
*/
//#define ADAFRUIT_ST7565_LCD

/************************************************************************/
/* LCD Pins                                                             */
/************************************************************************/

#define LCD_SID     10
#define LCD_SCK     11
#define LCD_A0      24
#define LCD_nRST    25
#define LCD_nCS     8

/************************************************************************/
/* Commands                                                             */
/************************************************************************/

#define BLACK 1
#define WHITE 0

#define ST7565_LCD_CMD_DISPLAY_OFF           0xAE
#define ST7565_LCD_CMD_DISPLAY_ON            0xAF

#define ST7565_LCD_CMD_SET_DISP_START_LINE   0x40
#define ST7565_LCD_CMD_SET_PAGE              0xB0

#define ST7565_LCD_CMD_SET_COLUMN_UPPER      0x10
#define ST7565_LCD_CMD_SET_COLUMN_LOWER      0x00

#define ST7565_LCD_CMD_SET_ADC_NORMAL        0xA0
#define ST7565_LCD_CMD_SET_ADC_REVERSE       0xA1

#define ST7565_LCD_CMD_SET_DISP_NORMAL       0xA6
#define ST7565_LCD_CMD_SET_DISP_REVERSE      0xA7

#define ST7565_LCD_CMD_SET_ALLPTS_NORMAL     0xA4
#define ST7565_LCD_CMD_SET_ALLPTS_ON         0xA5
#define ST7565_LCD_CMD_SET_BIAS_9            0xA2
#define ST7565_LCD_CMD_SET_BIAS_7            0xA3

#define ST7565_LCD_CMD_RMW                   0xE0
#define ST7565_LCD_CMD_RMW_CLEAR             0xEE
#define ST7565_LCD_CMD_INTERNAL_RESET        0xE2
#define ST7565_LCD_CMD_SET_COM_NORMAL        0xC0
#define ST7565_LCD_CMD_SET_COM_REVERSE       0xC8
#define ST7565_LCD_CMD_SET_POWER_CONTROL     0x28
#define ST7565_LCD_CMD_SET_RESISTOR_RATIO    0x20
#define ST7565_LCD_CMD_SET_VOLUME_FIRST      0x81
#define ST7565_LCD_CMD_SET_VOLUME_SECOND     0x0
#define ST7565_LCD_CMD_SET_STATIC_OFF        0xAC
#define ST7565_LCD_CMD_SET_STATIC_ON         0xAD
#define ST7565_LCD_CMD_SET_STATIC_REG        0x0
#define ST7565_LCD_CMD_SET_BOOSTER_FIRST     0xF8
#define ST7565_LCD_CMD_SET_BOOSTER_234       0
#define ST7565_LCD_CMD_SET_BOOSTER_5         1
#define ST7565_LCD_CMD_SET_BOOSTER_6         3
#define ST7565_LCD_CMD_NOP                   0xE3
#define ST7565_LCD_CMD_TEST                  0xF0

/************************************************************************/
/* Font Definitions                                                     */
/************************************************************************/
/**
 * 5x7 LCD font 'flipped' for the ST7565 - public domain
 * @note This is a 256 character font. Delete glyphs in order to save Flash
 */
/* Controls the definition of the Font array and character spcing */
//#define FULL_FONT
static const uint8_t gca_font[] =
{
#ifdef FULL_FONT
  0x0, 0x0, 0x0, 0x0, 0x0,          /* ASC(00) */
  0x7C, 0xDA, 0xF2, 0xDA, 0x7C,     /* ASC(01) */
  0x7C, 0xD6, 0xF2, 0xD6, 0x7C,     /* ASC(02) */
  0x38, 0x7C, 0x3E, 0x7C, 0x38,   /* ASC(03) */
  0x18, 0x3C, 0x7E, 0x3C, 0x18,   /* ASC(04) */
  0x38, 0xEA, 0xBE, 0xEA, 0x38,   /* ASC(05) */
  0x38, 0x7A, 0xFE, 0x7A, 0x38,   /* ASC(06) */
  0x0, 0x18, 0x3C, 0x18, 0x0,   /* ASC(07) */
  0xFF, 0xE7, 0xC3, 0xE7, 0xFF,   /* ASC(08) */
  0x0, 0x18, 0x24, 0x18, 0x0,   /* ASC(09) */
  0xFF, 0xE7, 0xDB, 0xE7, 0xFF,   /* ASC(10) */
  0xC, 0x12, 0x5C, 0x60, 0x70,   /* ASC(11) */
  0x64, 0x94, 0x9E, 0x94, 0x64,   /* ASC(12) */
  0x2, 0xFE, 0xA0, 0xA0, 0xE0,   /* ASC(13) */
  0x2, 0xFE, 0xA0, 0xA4, 0xFC,   /* ASC(14) */
  0x5A, 0x3C, 0xE7, 0x3C, 0x5A,   /* ASC(15) */
  0xFE, 0x7C, 0x38, 0x38, 0x10,   /* ASC(16) */
  0x10, 0x38, 0x38, 0x7C, 0xFE,   /* ASC(17) */
  0x28, 0x44, 0xFE, 0x44, 0x28,   /* ASC(18) */
  0xFA, 0xFA, 0x0, 0xFA, 0xFA,   /* ASC(19) */
  0x60, 0x90, 0xFE, 0x80, 0xFE,   /* ASC(20) */
  0x0, 0x66, 0x91, 0xA9, 0x56,   /* ASC(21) */
  0x6, 0x6, 0x6, 0x6, 0x6,   /* ASC(22) */
  0x29, 0x45, 0xFF, 0x45, 0x29,   /* ASC(23) */
  0x10, 0x20, 0x7E, 0x20, 0x10,   /* ASC(24) */
  0x8, 0x4, 0x7E, 0x4, 0x8,   /* ASC(25) */
  0x10, 0x10, 0x54, 0x38, 0x10,   /* ASC(26) */
  0x10, 0x38, 0x54, 0x10, 0x10,   /* ASC(27) */
  0x78, 0x8, 0x8, 0x8, 0x8,   /* ASC(28) */
  0x30, 0x78, 0x30, 0x78, 0x30,   /* ASC(29) */
  0xC, 0x1C, 0x7C, 0x1C, 0xC,   /* ASC(30) */
  0x60, 0x70, 0x7C, 0x70, 0x60,   /* ASC(31) */
#endif
    0x0, 0x0, 0x0, 0x0, 0x0, /* ASC(32) */
    0x0, 0x0, 0xFA, 0x0, 0x0, /* ASC(33) */
    0x0, 0xE0, 0x0, 0xE0, 0x0, /* ASC(34) */
    0x28, 0xFE, 0x28, 0xFE, 0x28, /* ASC(35) */
    0x24, 0x54, 0xFE, 0x54, 0x48, /* ASC(36) */
    0xC4, 0xC8, 0x10, 0x26, 0x46, /* ASC(37) */
    0x6C, 0x92, 0x6A, 0x4, 0xA, /* ASC(38) */
    0x0, 0x10, 0xE0, 0xC0, 0x0, /* ASC(39) */
    0x0, 0x38, 0x44, 0x82, 0x0, /* ASC(40) */
    0x0, 0x82, 0x44, 0x38, 0x0, /* ASC(41) */
    0x54, 0x38, 0xFE, 0x38, 0x54, /* ASC(42) */
    0x10, 0x10, 0x7C, 0x10, 0x10, /* ASC(43) */
    0x0, 0x1, 0xE, 0xC, 0x0, /* ASC(44) */
    0x10, 0x10, 0x10, 0x10, 0x10, /* ASC(45) */
    0x0, 0x0, 0x6, 0x6, 0x0, /* ASC(46) */
    0x4, 0x8, 0x10, 0x20, 0x40, /* ASC(47) */
    0x7C, 0x8A, 0x92, 0xA2, 0x7C, /* ASC(48) */
    0x0, 0x42, 0xFE, 0x2, 0x0, /* ASC(49) */
    0x4E, 0x92, 0x92, 0x92, 0x62, /* ASC(50) */
    0x84, 0x82, 0x92, 0xB2, 0xCC, /* ASC(51) */
    0x18, 0x28, 0x48, 0xFE, 0x8, /* ASC(52) */
    0xE4, 0xA2, 0xA2, 0xA2, 0x9C, /* ASC(53) */
    0x3C, 0x52, 0x92, 0x92, 0x8C, /* ASC(54) */
    0x82, 0x84, 0x88, 0x90, 0xE0, /* ASC(55) */
    0x6C, 0x92, 0x92, 0x92, 0x6C, /* ASC(56) */
    0x62, 0x92, 0x92, 0x94, 0x78, /* ASC(57) */
    0x0, 0x0, 0x28, 0x0, 0x0, /* ASC(58) */
    0x0, 0x2, 0x2C, 0x0, 0x0, /* ASC(59) */
    0x0, 0x10, 0x28, 0x44, 0x82, /* ASC(60) */
    0x28, 0x28, 0x28, 0x28, 0x28, /* ASC(61) */
    0x0, 0x82, 0x44, 0x28, 0x10, /* ASC(62) */
    0x40, 0x80, 0x9A, 0x90, 0x60, /* ASC(63) */
    0x7C, 0x82, 0xBA, 0x9A, 0x72, /* ASC(64) */
    0x3E, 0x48, 0x88, 0x48, 0x3E, /* ASC(65) */
    0xFE, 0x92, 0x92, 0x92, 0x6C, /* ASC(66) */
    0x7C, 0x82, 0x82, 0x82, 0x44, /* ASC(67) */
    0xFE, 0x82, 0x82, 0x82, 0x7C, /* ASC(68) */
    0xFE, 0x92, 0x92, 0x92, 0x82, /* ASC(69) */
    0xFE, 0x90, 0x90, 0x90, 0x80, /* ASC(70) */
    0x7C, 0x82, 0x82, 0x8A, 0xCE, /* ASC(71) */
    0xFE, 0x10, 0x10, 0x10, 0xFE, /* ASC(72) */
    0x0, 0x82, 0xFE, 0x82, 0x0, /* ASC(73) */
    0x4, 0x2, 0x82, 0xFC, 0x80, /* ASC(74) */
    0xFE, 0x10, 0x28, 0x44, 0x82, /* ASC(75) */
    0xFE, 0x2, 0x2, 0x2, 0x2, /* ASC(76) */
    0xFE, 0x40, 0x38, 0x40, 0xFE, /* ASC(77) */
    0xFE, 0x20, 0x10, 0x8, 0xFE, /* ASC(78) */
    0x7C, 0x82, 0x82, 0x82, 0x7C, /* ASC(79) */
    0xFE, 0x90, 0x90, 0x90, 0x60, /* ASC(80) */
    0x7C, 0x82, 0x8A, 0x84, 0x7A, /* ASC(81) */
    0xFE, 0x90, 0x98, 0x94, 0x62, /* ASC(82) */
    0x64, 0x92, 0x92, 0x92, 0x4C, /* ASC(83) */
    0xC0, 0x80, 0xFE, 0x80, 0xC0, /* ASC(84) */
    0xFC, 0x2, 0x2, 0x2, 0xFC, /* ASC(85) */
    0xF8, 0x4, 0x2, 0x4, 0xF8, /* ASC(86) */
    0xFC, 0x2, 0x1C, 0x2, 0xFC, /* ASC(87) */
    0xC6, 0x28, 0x10, 0x28, 0xC6, /* ASC(88) */
    0xC0, 0x20, 0x1E, 0x20, 0xC0, /* ASC(89) */
    0x86, 0x9A, 0x92, 0xB2, 0xC2, /* ASC(90) */
    0x0, 0xFE, 0x82, 0x82, 0x82, /* ASC(91) */
    0x40, 0x20, 0x10, 0x8, 0x4, /* ASC(92) */
    0x0, 0x82, 0x82, 0x82, 0xFE, /* ASC(93) */
    0x20, 0x40, 0x80, 0x40, 0x20, /* ASC(94) */
    0x2, 0x2, 0x2, 0x2, 0x2, /* ASC(95) */
    0x0, 0xC0, 0xE0, 0x10, 0x0, /* ASC(96) */
    0x4, 0x2A, 0x2A, 0x1E, 0x2, /* ASC(97) */
    0xFE, 0x14, 0x22, 0x22, 0x1C, /* ASC(98) */
    0x1C, 0x22, 0x22, 0x22, 0x14, /* ASC(99) */
    0x1C, 0x22, 0x22, 0x14, 0xFE, /* ASC(100) */
    0x1C, 0x2A, 0x2A, 0x2A, 0x18, /* ASC(101) */
    0x0, 0x10, 0x7E, 0x90, 0x40, /* ASC(102) */
    0x18, 0x25, 0x25, 0x39, 0x1E, /* ASC(103) */
    0xFE, 0x10, 0x20, 0x20, 0x1E, /* ASC(104) */
    0x0, 0x22, 0xBE, 0x2, 0x0, /* ASC(105) */
    0x4, 0x2, 0x2, 0xBC, 0x0, /* ASC(106) */
    0xFE, 0x8, 0x14, 0x22, 0x0, /* ASC(107) */
    0x0, 0x82, 0xFE, 0x2, 0x0, /* ASC(108) */
    0x3E, 0x20, 0x1E, 0x20, 0x1E, /* ASC(109) */
    0x3E, 0x10, 0x20, 0x20, 0x1E, /* ASC(110) */
    0x1C, 0x22, 0x22, 0x22, 0x1C, /* ASC(111) */
    0x3F, 0x18, 0x24, 0x24, 0x18, /* ASC(112) */
    0x18, 0x24, 0x24, 0x18, 0x3F, /* ASC(113) */
    0x3E, 0x10, 0x20, 0x20, 0x10, /* ASC(114) */
    0x12, 0x2A, 0x2A, 0x2A, 0x24, /* ASC(115) */
    0x20, 0x20, 0xFC, 0x22, 0x24, /* ASC(116) */
    0x3C, 0x2, 0x2, 0x4, 0x3E, /* ASC(117) */
    0x38, 0x4, 0x2, 0x4, 0x38, /* ASC(118) */
    0x3C, 0x2, 0xC, 0x2, 0x3C, /* ASC(119) */
    0x22, 0x14, 0x8, 0x14, 0x22, /* ASC(120) */
    0x32, 0x9, 0x9, 0x9, 0x3E, /* ASC(121) */
    0x22, 0x26, 0x2A, 0x32, 0x22, /* ASC(122) */
    0x0, 0x10, 0x6C, 0x82, 0x0, /* ASC(123) */
    0x0, 0x0, 0xEE, 0x0, 0x0, /* ASC(124) */
    0x0, 0x82, 0x6C, 0x10, 0x0, /* ASC(125) */
    0x40, 0x80, 0x40, 0x20, 0x40, /* ASC(126) */
#ifdef FULL_FONT
  0x3C, 0x64, 0xC4, 0x64, 0x3C,   /* ASC(127) */
  0x78, 0x85, 0x85, 0x86, 0x48,   /* ASC(128) */
  0x5C, 0x2, 0x2, 0x4, 0x5E,   /* ASC(129) */
  0x1C, 0x2A, 0x2A, 0xAA, 0x9A,   /* ASC(130) */
  0x84, 0xAA, 0xAA, 0x9E, 0x82,   /* ASC(131) */
  0x84, 0x2A, 0x2A, 0x1E, 0x82,   /* ASC(132) */
  0x84, 0xAA, 0x2A, 0x1E, 0x2,   /* ASC(133) */
  0x4, 0x2A, 0xAA, 0x9E, 0x2,   /* ASC(134) */
  0x30, 0x78, 0x4A, 0x4E, 0x48,   /* ASC(135) */
  0x9C, 0xAA, 0xAA, 0xAA, 0x9A,   /* ASC(136) */
  0x9C, 0x2A, 0x2A, 0x2A, 0x9A,   /* ASC(137) */
  0x9C, 0xAA, 0x2A, 0x2A, 0x1A,   /* ASC(138) */
  0x0, 0x0, 0xA2, 0x3E, 0x82,   /* ASC(139) */
  0x0, 0x40, 0xA2, 0xBE, 0x42,   /* ASC(140) */
  0x0, 0x80, 0xA2, 0x3E, 0x2,   /* ASC(141) */
  0xF, 0x94, 0x24, 0x94, 0xF,   /* ASC(142) */
  0xF, 0x14, 0xA4, 0x14, 0xF,   /* ASC(143) */
  0x3E, 0x2A, 0xAA, 0xA2, 0x0,   /* ASC(144) */
  0x4, 0x2A, 0x2A, 0x3E, 0x2A,   /* ASC(145) */
  0x3E, 0x50, 0x90, 0xFE, 0x92,   /* ASC(146) */
  0x4C, 0x92, 0x92, 0x92, 0x4C,   /* ASC(147) */
  0x4C, 0x12, 0x12, 0x12, 0x4C,   /* ASC(148) */
  0x4C, 0x52, 0x12, 0x12, 0xC,   /* ASC(149) */
  0x5C, 0x82, 0x82, 0x84, 0x5E,   /* ASC(150) */
  0x5C, 0x42, 0x2, 0x4, 0x1E,   /* ASC(151) */
  0x0, 0xB9, 0x5, 0x5, 0xBE,   /* ASC(152) */
  0x9C, 0x22, 0x22, 0x22, 0x9C,   /* ASC(153) */
  0xBC, 0x2, 0x2, 0x2, 0xBC,   /* ASC(154) */
  0x3C, 0x24, 0xFF, 0x24, 0x24,   /* ASC(155) */
  0x12, 0x7E, 0x92, 0xC2, 0x66,   /* ASC(156) */
  0xD4, 0xF4, 0x3F, 0xF4, 0xD4,   /* ASC(157) */
  0xFF, 0x90, 0x94, 0x6F, 0x4,   /* ASC(158) */
  0x3, 0x11, 0x7E, 0x90, 0xC0,   /* ASC(159) */
  0x4, 0x2A, 0x2A, 0x9E, 0x82,   /* ASC(160) */
  0x0, 0x0, 0x22, 0xBE, 0x82,   /* ASC(161) */
  0xC, 0x12, 0x12, 0x52, 0x4C,   /* ASC(162) */
  0x1C, 0x2, 0x2, 0x44, 0x5E,   /* ASC(163) */
  0x0, 0x5E, 0x50, 0x50, 0x4E,   /* ASC(164) */
  0xBE, 0xB0, 0x98, 0x8C, 0xBE,   /* ASC(165) */
  0x64, 0x94, 0x94, 0xF4, 0x14,   /* ASC(166) */
  0x64, 0x94, 0x94, 0x94, 0x64,   /* ASC(167) */
  0xC, 0x12, 0xB2, 0x2, 0x4,   /* ASC(168) */
  0x1C, 0x10, 0x10, 0x10, 0x10,   /* ASC(169) */
  0x10, 0x10, 0x10, 0x10, 0x1C,   /* ASC(170) */
  0xF4, 0x8, 0x13, 0x35, 0x5D,   /* ASC(171) */
  0xF4, 0x8, 0x14, 0x2C, 0x5F,   /* ASC(172) */
  0x0, 0x0, 0xDE, 0x0, 0x0,   /* ASC(173) */
  0x10, 0x28, 0x54, 0x28, 0x44,   /* ASC(174) */
  0x44, 0x28, 0x54, 0x28, 0x10,   /* ASC(175) */
  0x55, 0x0, 0xAA, 0x0, 0x55,   /* ASC(176) */
  0x55, 0xAA, 0x55, 0xAA, 0x55,   /* ASC(177) */
  0xAA, 0x55, 0xAA, 0x55, 0xAA,   /* ASC(178) */
  0x0, 0x0, 0x0, 0xFF, 0x0,   /* ASC(179) */
  0x8, 0x8, 0x8, 0xFF, 0x0,   /* ASC(180) */
  0x28, 0x28, 0x28, 0xFF, 0x0,   /* ASC(181) */
  0x8, 0x8, 0xFF, 0x0, 0xFF,   /* ASC(182) */
  0x8, 0x8, 0xF, 0x8, 0xF,   /* ASC(183) */
  0x28, 0x28, 0x28, 0x3F, 0x0,   /* ASC(184) */
  0x28, 0x28, 0xEF, 0x0, 0xFF,   /* ASC(185) */
  0x0, 0x0, 0xFF, 0x0, 0xFF,   /* ASC(186) */
  0x28, 0x28, 0x2F, 0x20, 0x3F,   /* ASC(187) */
  0x28, 0x28, 0xE8, 0x8, 0xF8,   /* ASC(188) */
  0x8, 0x8, 0xF8, 0x8, 0xF8,   /* ASC(189) */
  0x28, 0x28, 0x28, 0xF8, 0x0,   /* ASC(190) */
  0x8, 0x8, 0x8, 0xF, 0x0,   /* ASC(191) */
  0x0, 0x0, 0x0, 0xF8, 0x8,   /* ASC(192) */
  0x8, 0x8, 0x8, 0xF8, 0x8,   /* ASC(193) */
  0x8, 0x8, 0x8, 0xF, 0x8,   /* ASC(194) */
  0x0, 0x0, 0x0, 0xFF, 0x8,   /* ASC(195) */
  0x8, 0x8, 0x8, 0x8, 0x8,   /* ASC(196) */
  0x8, 0x8, 0x8, 0xFF, 0x8,   /* ASC(197) */
  0x0, 0x0, 0x0, 0xFF, 0x28,   /* ASC(198) */
  0x0, 0x0, 0xFF, 0x0, 0xFF,   /* ASC(199) */
  0x0, 0x0, 0xF8, 0x8, 0xE8,   /* ASC(200) */
  0x0, 0x0, 0x3F, 0x20, 0x2F,   /* ASC(201) */
  0x28, 0x28, 0xE8, 0x8, 0xE8,   /* ASC(202) */
  0x28, 0x28, 0x2F, 0x20, 0x2F,   /* ASC(203) */
  0x0, 0x0, 0xFF, 0x0, 0xEF,   /* ASC(204) */
  0x28, 0x28, 0x28, 0x28, 0x28,   /* ASC(205) */
  0x28, 0x28, 0xEF, 0x0, 0xEF,   /* ASC(206) */
  0x28, 0x28, 0x28, 0xE8, 0x28,   /* ASC(207) */
  0x8, 0x8, 0xF8, 0x8, 0xF8,   /* ASC(208) */
  0x28, 0x28, 0x28, 0x2F, 0x28,   /* ASC(209) */
  0x8, 0x8, 0xF, 0x8, 0xF,   /* ASC(210) */
  0x0, 0x0, 0xF8, 0x8, 0xF8,   /* ASC(211) */
  0x0, 0x0, 0x0, 0xF8, 0x28,   /* ASC(212) */
  0x0, 0x0, 0x0, 0x3F, 0x28,   /* ASC(213) */
  0x0, 0x0, 0xF, 0x8, 0xF,   /* ASC(214) */
  0x8, 0x8, 0xFF, 0x8, 0xFF,   /* ASC(215) */
  0x28, 0x28, 0x28, 0xFF, 0x28,   /* ASC(216) */
  0x8, 0x8, 0x8, 0xF8, 0x0,   /* ASC(217) */
  0x0, 0x0, 0x0, 0xF, 0x8,   /* ASC(218) */
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   /* ASC(219) */
  0xF, 0xF, 0xF, 0xF, 0xF,   /* ASC(220) */
  0xFF, 0xFF, 0xFF, 0x0, 0x0,   /* ASC(221) */
  0x0, 0x0, 0x0, 0xFF, 0xFF,   /* ASC(222) */
  0xF0, 0xF0, 0xF0, 0xF0, 0xF0,   /* ASC(223) */
  0x1C, 0x22, 0x22, 0x1C, 0x22,   /* ASC(224) */
  0x3E, 0x54, 0x54, 0x7C, 0x28,   /* ASC(225) */
  0x7E, 0x40, 0x40, 0x60, 0x60,   /* ASC(226) */
  0x40, 0x7E, 0x40, 0x7E, 0x40,   /* ASC(227) */
  0xC6, 0xAA, 0x92, 0x82, 0xC6,   /* ASC(228) */
  0x1C, 0x22, 0x22, 0x3C, 0x20,   /* ASC(229) */
  0x2, 0x7E, 0x4, 0x78, 0x4,   /* ASC(230) */
  0x60, 0x40, 0x7E, 0x40, 0x40,   /* ASC(231) */
  0x99, 0xA5, 0xE7, 0xA5, 0x99,   /* ASC(232) */
  0x38, 0x54, 0x92, 0x54, 0x38,   /* ASC(233) */
  0x32, 0x4E, 0x80, 0x4E, 0x32,   /* ASC(234) */
  0xC, 0x52, 0xB2, 0xB2, 0xC,   /* ASC(235) */
  0xC, 0x12, 0x1E, 0x12, 0xC,   /* ASC(236) */
  0x3D, 0x46, 0x5A, 0x62, 0xBC,   /* ASC(237) */
  0x7C, 0x92, 0x92, 0x92, 0x0,   /* ASC(238) */
  0x7E, 0x80, 0x80, 0x80, 0x7E,   /* ASC(239) */
  0x54, 0x54, 0x54, 0x54, 0x54,   /* ASC(240) */
  0x22, 0x22, 0xFA, 0x22, 0x22,   /* ASC(241) */
  0x2, 0x8A, 0x52, 0x22, 0x2,   /* ASC(242) */
  0x2, 0x22, 0x52, 0x8A, 0x2,   /* ASC(243) */
  0x0, 0x0, 0xFF, 0x80, 0xC0,   /* ASC(244) */
  0x7, 0x1, 0xFF, 0x0, 0x0,   /* ASC(245) */
  0x10, 0x10, 0xD6, 0xD6, 0x10,   /* ASC(246) */
  0x6C, 0x48, 0x6C, 0x24, 0x6C,   /* ASC(247) */
  0x60, 0xF0, 0x90, 0xF0, 0x60,   /* ASC(248) */
  0x0, 0x0, 0x18, 0x18, 0x0,   /* ASC(249) */
  0x0, 0x0, 0x8, 0x8, 0x0,   /* ASC(250) */
  0xC, 0x2, 0xFF, 0x80, 0x80,   /* ASC(251) */
  0x0, 0xF8, 0x80, 0x80, 0x78,   /* ASC(252) */
  0x0, 0x98, 0xB8, 0xE8, 0x48,   /* ASC(253) */
  0x0, 0x3C, 0x3C, 0x3C, 0x3C,   /* ASC(254) */
#endif
    };

/************************************************************************/
/* LCD Parameters                                                       */
/************************************************************************/

#define ST7565_LCD_PARAM_WIDTH               128U
#define ST7565_LCD_PARAM_HEIGHT              64U
#define ST7565_LCD_PARAM_PAGEHEIGHT          8U
/* Rows max (ST7565_LCD_PARAM_HEIGHT/ST7565_LCD_PARAM_PAGEHEIGHT) */
#define ST7565_LCD_MAX_ROWS                  8U
#define ST7565_LCD_MASK_ROWS                 0x07
#define ST7565_LCD_MAX_COLUMNS               ST7565_LCD_PARAM_WIDTH
#define ST7565_LCD_MASK_COLUMNS              0x7F

#ifdef ADAFRUIT_ST7565_LCD
#define ST7565_LCD_PARAM_BRIGHTNESS          0x18
#else
#define ST7565_LCD_PARAM_BRIGHTNESS          0x0
#endif

#define ST7565_LCD_PARAM_FONT_WIDTH          5
#define ST7565_LCD_PARAM_FONT_CHARWIDTH      7
#define ST7565_LCD_PARAM_FONT_HEIGHT         7
#define ST7565_LCD_PARAM_FONT_CHARHEIGHT     8
#ifndef FULL_FONT
#define ST7565_LCD_PARAM_FONT_CHAR_MINVAL    32
#define ST7565_LCD_PARAM_FONT_CHAR_MAXVAL    126
#else
#define ST7565_LCD_PARAM_FONT_CHAR_MINVAL    8
#define ST7565_LCD_PARAM_FONT_CHAR_MAXVAL    255
#endif   
#define ST7565_LCD_PARAM_SPISPEED            20000000UL

/************************************************************************/
/* LCD Format Characters                                                */
/************************************************************************/
/**
 * Screen Newline Character
 */
#define ST7565_LCD_FMT_NEWINE ('\n')
/**
 * Space Special Character Followed by <Number of Character Space>
 *  This helps to add a number of space characters
 *  The number is always in HEX code of 2 digits.
 *  
 *  Eg. "\x0105Testing"
 *    This would add 5 spaces by \x01-05 before pringing "Testing"
 */
#define ST7565_LCD_FMT_SPACE  ('\x01')
/**
 * Raw Format Special Character Followed by 
 *  <Low Byte Size> and <High Byte Size> Then the Raw 8bit Data 
 *  Both the sizes in Hex 2 digit number format
 *  The 8bit data bytes are also coded in string as Hex 2 digit numbers
 *  
 *  Eg. "\x02050011A1032F5A"
 *    In this example we have 5 bytes denoted by \x02-05-00
 *    High Byte = 0 Low Byte = 5
 *    And special direct graphic characters: 11-A1-03-2F-5A
 */
#define ST7565_LCD_FMT_RAW    ('\x02')
 /**
 * Font Specifier Special Character Followed by 1 Byte of Font Number
 * 
 * - Not Implemented Do not USE
 */
#define ST7565_LCD_FMT_FONT   ('\x03')
/**
 * Direct Set Co-ordinates Special Character Followed by 1 Byte of Column
 *  and 1 Byte of Row specification
 */
#define ST7565_LCD_FMT_COORDINATES   ('\x04')
/**
 * Direct Column Offset Special Character Followed by 
 *  1 Byte of Number of Columns to Space up
 */
#define ST7565_LCD_FMT_COLUMNOFFSET  ('\x05')
/**
 * Backspace Character for Correction - Would clear the previous character
 *  and Restore the Row/Column positions
 */
#define ST7565_LCD_FMT_BACKSPACE ('\x08')
/**
 * Cursor Visible Special Character - Make a cursor visible at the 
 *  current location - 128 is typically the Cursor
 */
#define ST7565_LCD_FMT_CURSOR ('\x80')

/************************************************************************/
/* GPIO Driver Functions                                                */
/************************************************************************/

/**
 *  Function to initialize the GPIO and SPI peripheral on Rpi
 *  irrespective of the LCD initialization.
 *  - This is the first function that needs to be called before any other
 *  operation can be performed.
 */
int init_io()
{
  if(gpioSetMode(LCD_A0, PI_OUTPUT) != 0) return -21;
  if(gpioSetMode(LCD_nRST, PI_OUTPUT) != 0) return -22;
  gx_spihandle = spiOpen(0, ST7565_LCD_PARAM_SPISPEED, 192);
  if(gx_spihandle > 0) /* Other Handles are Open then */
  {
    int i;
    for(i = 0; i < gx_spihandle; i++)
    {
        spiClose(i);
    }
    spiClose(gx_spihandle);
    /* Since all Handles have been closed */
    gx_spihandle = spiOpen(0, ST7565_LCD_PARAM_SPISPEED, 192);
  }
  if(gx_spihandle != 0) /* If Still there is some Issue*/
  {
    printf("\n ERROR: Invalid SPI handle %d \n", gx_spihandle);
    return -23;
  }
  return 0;
}

/************************************************************************/
/* LCD Functions                                                        */
/************************************************************************/

/**
 *  Function to send in one byte Command to the LCD
 */
int lcd_cmd(uint8_t byte)
{
  char buf[1] = {0};
  if(gx_spihandle != 0) return -41;
  buf[0] = (char)byte;
  if(gpioWrite(LCD_A0, 0) != 0) return -42;
  if(spiWrite(gx_spihandle, buf, 1) != 1) return -43;
  usleep(1);
  return 0;
}
/**
 *  Function to Send one byte of graphic data column (8 pixels) to LCD
 */
int lcd_data(uint8_t byte)
{
  char buf[1] = {0};
  if(gx_spihandle != 0) return -31;
  buf[0] = (char)byte;
  if(gpioWrite(LCD_A0, 1) != 0) return -32;
  if(spiWrite(gx_spihandle, buf, 1) != 1) return -33;
  usleep(1);
  return 0;
}
/**
 *  Function to Reset the LCD to its initial state
 */
int lcd_reset()
{
  if(gpioWrite(LCD_nRST, 0) != 0) return -51;
  usleep(500000); /* 500 ms*/
  if(gpioWrite(LCD_nRST, 1) != 0) return -52;
  return 0;
}
/**
 *  Function to control the Contrast and brightness ratio
 *  - This is generally a fixed value for a given LCD.
 */
int lcd_bright ( int bValue )
{
  int retcode = 0;
  retcode = lcd_cmd(ST7565_LCD_CMD_SET_VOLUME_FIRST);
  if(retcode != 0) return retcode;
  return lcd_cmd(ST7565_LCD_CMD_SET_VOLUME_SECOND | (bValue & 0x3f));
}
/**
 * @brief Function to position the draw cursor
 *    Need initialization of LCD @ref lcd_init before using this function
 *    This function also updates the cursor location
 *    Thre are special extention to support the normal and
 *    Adafruit specific LCD versions.
 * 
 * @param bColumn The X axis location from the Top Right will be 0
 * @param bRow The Y axis location from the Bottom Left will be 0
 * 
 * @return Status of the operation
 *        0 for successful operation
 *        -61 for Column error
 *        -62 for Row error
 */
int lcd_goto ( uint8_t bColumn, uint8_t bRow )
{
  if (bColumn >= ST7565_LCD_MAX_COLUMNS)
  {
    return -61;
  }
#ifdef ADAFRUIT_ST7565_LCD  
  if (bRow >= (ST7565_LCD_MAX_ROWS + 1))
#else      
  if (bRow >= (ST7565_LCD_MAX_ROWS))
#endif      
  {
    return -62;
  }

  /* Get the Values into the Global position storage for the Cursor */
  gw_row = bRow;
  gw_column = bColumn;

#ifdef ADAFRUIT_ST7565_LCD
  /* Set the LCD Row */
  lcd_cmd(ST7565_LCD_CMD_SET_PAGE | ((7 - (bRow & 0x7)) ^ 4));

  /* Set the LCD Column */
  ++bColumn; /* 1 Offset for 0th Line */
  lcd_cmd(ST7565_LCD_CMD_SET_COLUMN_LOWER | (bColumn & 0xf));
  /* 127 max */
  lcd_cmd(ST7565_LCD_CMD_SET_COLUMN_UPPER | ((bColumn >> 4) & 0x7)); 
#else
  /* Set the LCD Row */
  lcd_cmd(ST7565_LCD_CMD_SET_PAGE|(7-bRow));
  
  /* Set the LCD Column */
  lcd_cmd(ST7565_LCD_CMD_SET_COLUMN_LOWER | (bColumn & 0xf));
  /* 127 max */
  lcd_cmd(ST7565_LCD_CMD_SET_COLUMN_UPPER | ((bColumn >> 4) & 0x7)); 
#endif    
  return 0;
}
/**
 * @brief Function to Clear the Display graphic memory
 *    Need initialization of LCD @ref lcd_init before using this function
 *    This function also updates the cursor location
 * 
 * @param None
 * @return Status of the Operation
 *      0 for successful operation
 *      Else the Status of the @ref lcd_goto in case of error
 */
int lcd_clear ( void )
{
  uint8_t r, c;
  int retcode = 0;

  /* Go through each line of the display */
#ifdef ADAFRUIT_ST7565_LCD    
  for (r = 0; r < (ST7565_LCD_MAX_ROWS + 1) ; r++)
#else
  for (r = 0; r < ST7565_LCD_MAX_ROWS ; r++)
#endif
  {
    retcode = lcd_goto(0, r); /* Position cursor */
    if(retcode != 0) return retcode;
    /* For each column, write out a blank (white) byte */
    for (c = 0; c < ST7565_LCD_MAX_COLUMNS ; c++)
    {
      lcd_data(0x00);
    }
  }
  /* Set the Final Address at the Top Left Corner */
  return lcd_goto(0, 0);  
}

/**
 * @brief Function to Initialize the LCD driver with Reset
 * @details Before calling this we need to call @ref init_io
 *      - All previous status would be erased and display is refreshed
 *      - This needs to be done before writing/drawing any thing to the LCD
 * 
 * @param None
 * @return Status code for the Operaiton
 *        0 for successful operation
 *        -24 and -25 for error in I/O operations
 *        Else the last operation @ref lcd_clear
 */
int lcd_init()
{
  if(gpioWrite(LCD_A0, 0) != 0) return -24;
  if(gpioWrite(LCD_nRST, 0) != 0) return -25;
  /* Reset the LCD */
  lcd_reset();
   /* Send Commands */
  lcd_cmd(ST7565_LCD_CMD_SET_BIAS_7); /* Setup 1/7th Bias Level */
  lcd_cmd(ST7565_LCD_CMD_SET_ADC_NORMAL); /* ADC Select */
  lcd_cmd(ST7565_LCD_CMD_SET_COM_NORMAL); /* SHL Select */
  lcd_cmd(ST7565_LCD_CMD_SET_DISP_START_LINE); /* Initial Display Line */
  /* Turn On voltage converter (VC=1, VR=0, VF=0) */
  lcd_cmd(ST7565_LCD_CMD_SET_POWER_CONTROL | 0x4);
  usleep(50000); /* wait for 50% rising */
  /* Turn On voltage regulator (VC=1, VR=1, VF=0) */
  lcd_cmd(ST7565_LCD_CMD_SET_POWER_CONTROL | 0x6);
  usleep(50000);
  /* Turn on voltage follower (VC=1, VR=1, VF=1) */
  lcd_cmd(ST7565_LCD_CMD_SET_POWER_CONTROL | 0x7);
  usleep(10000); /* Wait */
  /* Set LCD operating voltage (regulator resistor, ref voltage resistor) */
  lcd_cmd(ST7565_LCD_CMD_SET_RESISTOR_RATIO | 0x7);
  lcd_cmd(ST7565_LCD_CMD_DISPLAY_ON);
  lcd_cmd(ST7565_LCD_CMD_SET_ALLPTS_NORMAL);
  lcd_bright(ST7565_LCD_PARAM_BRIGHTNESS);   
  
  /* Finally Clean up the LCD */
  return lcd_clear();
}
/**
 * @brief Function to Put the LCD into Deep Sleep mode
 * 
 * @param None
 */
void lcd_sleep ( void )
{
  lcd_cmd(ST7565_LCD_CMD_SET_STATIC_OFF);
  lcd_cmd(ST7565_LCD_CMD_DISPLAY_OFF);
  lcd_cmd(ST7565_LCD_CMD_SET_ALLPTS_ON);
}
/**
 * @brief Function to bring back the LCD from Deep Sleep mode
 *    All display contents are cleared and need to reinitalize
 *    
 * @param None
 */
void lcd_wakeup ( void )
{
  lcd_cmd(ST7565_LCD_CMD_INTERNAL_RESET);
  lcd_bright(ST7565_LCD_PARAM_BRIGHTNESS);
  lcd_cmd(ST7565_LCD_CMD_SET_ALLPTS_NORMAL);
  lcd_cmd(ST7565_LCD_CMD_DISPLAY_ON);
  lcd_cmd(ST7565_LCD_CMD_SET_STATIC_ON);
  lcd_cmd(ST7565_LCD_CMD_SET_STATIC_REG | 0x03);
}
/**
 * @brief Function to Put and wakup the LCD in Standby mode
 *      In this mode the display contents are not lost
 *      its only not visible
 * @param bWakeUp Used to decide that if we need to enter or exit standby
 *        0 - Enters into the Standby mode
 *        1 - Exits the Standby mode
 */
void lcd_standby ( uint8_t bWakeUp )
{
  if (bWakeUp == 0)
  { /* Enter standby mode */
    lcd_cmd(ST7565_LCD_CMD_SET_STATIC_ON);
    lcd_cmd(ST7565_LCD_CMD_SET_STATIC_REG | 0x03);
    lcd_cmd(ST7565_LCD_CMD_DISPLAY_OFF);
    lcd_cmd(ST7565_LCD_CMD_SET_ALLPTS_ON);
  }
  else
  { /* Wake from standby */
    lcd_cmd(ST7565_LCD_CMD_SET_ALLPTS_NORMAL);
    lcd_cmd(ST7565_LCD_CMD_DISPLAY_ON);
  }
}
static void _lcd_process_putc_newline ()
{
  /* Increment and Set the Location of Cursor */
  ++gw_row;
  if (gw_row >= ST7565_LCD_MAX_ROWS) /* Check if we are at the edge of the Screen */
  {
    gw_row = 0;
  }
  lcd_goto(0, gw_row);
}

void lcd_putc ( char c )
{
  uint8_t data, i;
  uint8_t *pFont;
  unsigned uoffset;

  data = (uint8_t) ((uint8_t) c & 0x7FU); /* Filter out the Higher Range */
  /* Filter our the Lower Range */
  if (data < ST7565_LCD_PARAM_FONT_CHAR_MINVAL)
  {
    if (data == '\n') /* process the New Line */
    {
      _lcd_process_putc_newline();
    }
    else if (data == ST7565_LCD_FMT_CURSOR)
    {
      /* Put Cursor */
      for (i = ST7565_LCD_PARAM_FONT_CHARWIDTH; i > 0 ; i--)
      {
        lcd_data(0xFC);
      }
    }
    return;
  }

  /* Get the Cursor shift for the current character - Need to avoid broken character writes */
  gw_column += ST7565_LCD_PARAM_FONT_CHARWIDTH;

  /* Check if we have spilled over the boundary */
  if (gw_column >= ST7565_LCD_MAX_COLUMNS) 
  {
    _lcd_process_putc_newline();
    /* Still needed as the current character would be written that 
       the new line first Place*/
    gw_column += ST7565_LCD_PARAM_FONT_CHARWIDTH; 
  }

  /* Compute the Font Offset - We need 16 bit offset for actual address 
   computation so using a pointer as a 16bit storage */
  uoffset = (unsigned) (((uint16_t)data - ST7565_LCD_PARAM_FONT_CHAR_MINVAL) * 
    ST7565_LCD_PARAM_FONT_WIDTH);

  /* Get the Font Pointer */
  pFont =  (uint8_t *) ((uint8_t *)gca_font + uoffset);

  /* Get the Font to Screen */
  for (i = ST7565_LCD_PARAM_FONT_WIDTH; i > 0 ; i--, pFont++)
  {
    data = *pFont;
    lcd_data(data);
  }
  /* For the Additional Char width to have spacing */
  for (i = (ST7565_LCD_PARAM_FONT_CHARWIDTH - ST7565_LCD_PARAM_FONT_WIDTH); 
    i > 0 ; i--)
  {
    lcd_data(0x00);
  }
}

/************************************************************************/
/* Actual Execution                                                     */
/************************************************************************/

/**
 * @brief Main Function
 * 
 * @param argc Number of Arguments
 * @param argv Passed Argument values in an Array
 * 
 * @return Program final Return status
 */
int main(int argc,char **argv)
{
  int retcode = 0;
  /* initialize the Driver */
  if(gpioInitialise() < 0)
  {
      printf("\n ERROR: Could not initialize the GPIO \n");
      return -1;
  }
  //printf("\n argc=%d\n", argc);
  /* Enter Processing Loop */
  do{
    /* initialize the I/O for Communications */
    retcode = init_io();
    if(retcode != 0) break;
    
    /* Based on Input Codes perform the Function */
    
    /* In case of bare minimum input or 'init' command */
    if(argc == 1 || (argc >= 2 && (strcmp("init", argv[1]) == 0) ))
    {
      /* Initialize the LCD */
      retcode = lcd_init();
      break;
    }
    
    if(strcmp("sleep", argv[1]) == 0)
    {
       /* Enter into Standby */
      lcd_standby(0);
      break;
    }

    if(strcmp("wakeup", argv[1]) == 0)
    {
      /* Exit into Standby */
      lcd_standby(1);
      break;
    }

    if(strcmp("c", argv[1]) == 0)
    {
      /* Clear the LCD */
      retcode = lcd_clear();
      break;
    }

    if((strcmp("g", argv[1]) == 0) && argc == 4)
    {
      uint8_t row,col;
      col = (uint8_t) (atoi(argv[2]) & ST7565_LCD_MASK_COLUMNS);
      row = (uint8_t) (atoi(argv[3]) & ST7565_LCD_MASK_ROWS);
      /* Go the Specific LCD Location */
      retcode = lcd_goto(col, row);
      break;
    }

    if(strcmp("test", argv[1]) == 0)
    {
      int i;
      /* Test pattern */
      for(i=0;i<64;i++)
      {
        lcd_data(0x55);
        lcd_data(0xAA);
        lcd_data(0x55);
        lcd_data(0xAA);
        lcd_data(0x55);
        lcd_data(0xAA);
      }
      break;
    }

    if((strcmp("w", argv[1]) == 0) && argc == 3)      
    {
      /* Print text in the Current Location */
      int i;
      for(i=0;argv[2][i]!=0;i++) /* Null Terminated String */
      {
        lcd_putc(argv[2][i]);
      }
      break;
    }
    
    /* Print the Help Text */
    printf("\n  Grapics LCD driver for ST7565 based 128 x 64 B/W LCD ");
    printf("\n ------------------------------------------------------\n");
    printf("\n Usage: ");
    printf("\n     sudo ./lcd init  - Initialize the LCD ");
    printf("\n     sudo ./lcd c     - Clear the LCD screen");
    printf("\n     sudo ./lcd g X Y - Set the LCD write ");
    printf("location to X(Column) and Y(Row)");
    printf("\n     sudo ./lcd w \"String\" - Used to Print a string on LCD ");
    printf("\n     sudo ./lcd test  - Draw a pattern on the LCD at the");
    printf(" current location ");
    printf("\n     sudo ./lcd sleep  - Put LCD in Sleep mode ");
    printf("\n     sudo ./lcd wakeup - Start the LCD from Sleep mode ");
    printf("\n");
    printf("\n   Design by Boseji <prog.ic@live.in> \n\n");
    
    
  }while(0);
  /* Check if the SPI handle is open */
  if(gx_spihandle >= 0)
  {
    gx_spihandle = spiClose(gx_spihandle);
  }
  /* Terminate the Driver */
  gpioTerminate();
  /* In Error scenarios print the Last Return code*/
  if( retcode != 0 )
    printf("\nError Code: %d\n", retcode);
  return retcode;
}
