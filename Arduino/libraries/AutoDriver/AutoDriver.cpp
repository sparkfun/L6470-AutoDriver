#include "AutoDriver.h"
#include "util/delay.h" // Turns out, using the Arduino "delay" function
                        //  in a library constructor causes the program to
                        //  hang if the constructor is invoked outside of
                        //  setup() or hold() (i.e., the user attempts to
                        //  create a global of the class.

// Constructors
AutoDriver::AutoDriver(int CSPin, int resetPin, int busyPin)
{
  _CSPin = CSPin;
  _resetPin = resetPin;
  _busyPin = busyPin;
  
  SPIConfig();
}

AutoDriver::AutoDriver(int CSPin, int resetPin)
{
  _CSPin = CSPin;
  _resetPin = resetPin;
  _busyPin = -1;

  SPIConfig();
}

void AutoDriver::SPIConfig()
{
  pinMode(11, OUTPUT); //MOSI
  pinMode(12, INPUT);  //MISO
  pinMode(13, OUTPUT); //SCK
  pinMode(_CSPin, OUTPUT);
  digitalWrite(_CSPin, HIGH);
  pinMode(_resetPin, OUTPUT);
  if (_busyPin != -1) pinMode(_busyPin, INPUT_PULLUP);
  
    // Let's set up the SPI peripheral. SPCR first:
  //  bit 7 - SPI interrupt (disable)
  //  bit 6 - SPI peripheral enable (enable)
  //  bit 5 - data order (MSb first)
  //  bit 4 - master/slave select (master mode)
  //  bit 3 - CPOL (active high)
  //  bit 2 - CPHA (sample on falling edge)
  //  bit 1:0 - data rate (8 or 16, depending on SPSR0)
  SPCR = B01011101;
  
  // SPSR next- not much here, just SPI2X
  //  bit 0 - double clock rate (no)
  SPSR = B00000000;
  
  // From now on, any data written to SPDR will be pumped out over the SPI pins
  //  and SPSR7 will be set when data TX/RX is complete. Read SPSR, then SPDR, to
  //  clear.
  digitalWrite(_resetPin, LOW);
  _delay_ms(5);
  digitalWrite(_resetPin, HIGH);
  _delay_ms(5);
}

int AutoDriver::busyCheck(void)
{
  if (_busyPin == -1)
  {
    if (getParam(STATUS) & 0x0002) return 0;
    else                           return 1;
  }
  else 
  {
    if (digitalRead(_busyPin) == HIGH) return 0;
    else                               return 1;
  }
}
