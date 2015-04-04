# Rpi-ST7565-SPI-LCD

This project deals with Interfacing a ST7565 SPI based Graphic LCD to Raspberry Pi using GPIO and SPI peripherals.

We use the SPI0 interface on the Raspberry Pi P1/J1 connector along with a couble of GPIO pins to control the LCD.
The backlight is connected using a 150ohm 2W resistance to give decent backlight for the LCD.

Here is the pin configuraiton:

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
    GPIO08(SPI_CE0N) - nCS


Using the [`pigpio`](http://abyz.co.uk/rpi/pigpio/download.html) we were able to prepare an application driver 
to interface to this LCD.

This program help to initialize, write and draw graphic on this LCD via a command line interface on Raspberry Pi.

The commercially available LCD part:

**Adafruit White LED ST7565 LCD**
  
Product: [http://www.adafruit.com/products/250](http://www.adafruit.com/products/250)
    
LCD Data-sheet: http://goo.gl/pZO0Ng

Most of the details are packed up with this program.

****
The next steps would be integrate this driver into the Frame Buffer kernel driver inside Raspberry Pi.

