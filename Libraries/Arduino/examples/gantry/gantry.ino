/****************************************************************************
 * gantry.ino
 * Five-axis gantry example for Autodriver
 * Mike Hord @ SparkFun Electronics
 * 16 Aug 2016
 * https://github.com/sparkfun/SparkFun_AutoDriver_Arduino_Library
 * 
 * This file demonstrates the use of five Autodriver boards in one sketch, to
 * control a five-axis gantry system such as one might find on a 3D printer.
 * 
 * Development environment specifics:
 * Arduino 1.6.9
 * SparkFun Arduino Pro board, Autodriver V13
 * 
 * This code is beerware; if you see me (or any other SparkFun employee) at the
 * local, and you've found our code helpful, please buy us a round!
 * ****************************************************************************/

#include <SparkFunAutoDriver.h>
#include <SPI.h>

#define NUM_BOARDS 5

// The gantry we're using has five autodriver boards, but
//  only four of them are in use. The board in position 3
//  is reserved for future use.

// Numbering starts from the board farthest from the
//  controller and counts up from 0. Thus, board "4" is the
//  board which is plugged directly into the controller
//  and board "0" is the farthest away.
AutoDriver YAxis(4, 10, 6);
AutoDriver Unused(3, 10, 6);
AutoDriver ZAAxis(2, 10, 6);
AutoDriver ZBAxis(1, 10, 6);
AutoDriver XAxis(0, 10, 6);

// It may be useful to index over the drivers, say, to set
//  the values that should be all the same across all the
//  boards. Let's make an array of pointers to AutoDriver
//  objects so we can do just that.
AutoDriver *boardIndex[NUM_BOARDS];

void setup() 
{
  Serial.begin(115200);
  Serial.println("Hello world!");
  
  // Start by setting up the SPI port and pins. The
  //  Autodriver library does not do this for you!
  pinMode(6, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(13, OUTPUT);
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  digitalWrite(6, LOW);
  digitalWrite(6, HIGH);
  SPI.begin();
  SPI.setDataMode(SPI_MODE3);

  // Set the pointers in our global AutoDriver array to
  //  the objects we've created.
  boardIndex[4] = &YAxis;
  boardIndex[3] = &Unused;
  boardIndex[2] = &ZAAxis;
  boardIndex[1] = &ZBAxis;
  boardIndex[0] = &XAxis;

  // Configure the boards to the settings we wish to use
  //  for them. See below for more information about the
  //  settings applied.
  configureBoards();
}

// loop() is going to wait to receive a character from the
//  host, then do something based on that character.
void loop() 
{
  char rxChar = 0;
  if (Serial.available())
  {
    rxChar = Serial.read();
    // This switch statement handles the various cases
    //  that may come from the host via the serial port.
    //  Do note that not all terminal programs will
    //  encode e.g. Page Up as 0x0B. I'm using CoolTerm
    //  in Windows and this is what CoolTerm sends when
    //  I strike these respective keys.
    switch(rxChar)
    {
      case 0x0B:  // Page Up
        ZAAxis.run(1, 600);
        ZBAxis.run(1, 600);
        delay(500);
        ZAAxis.run(1, 0);
        ZBAxis.run(1, 0);
        break;
      case 0x0C:  // Page Down
        ZAAxis.run(0, 600);
        ZBAxis.run(0, 600);
        delay(500);
        ZAAxis.run(0, 0);
        ZBAxis.run(0, 0);
        break;
      case 0x1E:  // Up Arrow
        YAxis.run(1,200);
        delay(500);
        YAxis.run(0,0);
        break;
      case 0x1F:  // Down Arrow
        YAxis.run(0,200);
        delay(500);
        YAxis.run(0,0);
        break;
      case 0x1C:  // Left Arrow
        XAxis.run(1,200);
        delay(500);
        XAxis.run(1,0);
        break;
      case 0x1D:  // Right Arrow
        XAxis.run(0,200);
        delay(500);
        XAxis.run(0,0);
        break;
      case 'g':
        getBoardConfigurations();
        break;
      case 'c':
        break;
      default:
        Serial.println("Command not found.");
    }
  }
}

// For ease of reading, we're just going to configure all the boards
//  to the same settings. It's working okay for me.
void configureBoards()
{
  int paramValue;
  Serial.println("Configuring boards...");
  for (int i = 0; i < NUM_BOARDS; i++)
  {
    // Before we do anything, we need to tell each board which SPI
    //  port we're using. Most of the time, there's only the one,
    //  but it's possible for some larger Arduino boards to have more
    //  than one, so don't take it for granted.
    boardIndex[i]->SPIPortConnect(&SPI);
    
    // Set the Overcurrent Threshold to 6A. The OC detect circuit
    //  is quite sensitive; even if the current is only momentarily
    //  exceeded during acceleration or deceleration, the driver
    //  will shutdown. This is a per channel value; it's useful to
    //  consider worst case, which is startup. These motors have a
    //  1.8 ohm static winding resistance; at 12V, the current will
    //  exceed this limit, so we need to use the KVAL settings (see
    //  below) to trim that value back to a safe level.
    boardIndex[i]->setOCThreshold(OCD_TH_6000mA);
    
    // KVAL is a modifier that sets the effective voltage applied
    //  to the motor. KVAL/255 * Vsupply = effective motor voltage.
    //  This lets us hammer the motor harder during some phases
    //  than others, and to use a higher voltage to achieve better
    //  torqure performance even if a motor isn't rated for such a
    //  high current. This system has 3V motors and a 12V supply.
    boardIndex[i]->setRunKVAL(192);  // 128/255 * 12V = 6V
    boardIndex[i]->setAccKVAL(192);  // 192/255 * 12V = 9V
    boardIndex[i]->setDecKVAL(192);
    boardIndex[i]->setHoldKVAL(32);  // 32/255 * 12V = 1.5V
    
    // When a move command is issued, max speed is the speed the
    //  motor tops out at while completing the move, in steps/s
    boardIndex[i]->setMaxSpeed(300);
    
    // Acceleration and deceleration in steps/s/s. Increasing this
    //  value makes the motor reach its full speed more quickly,
    //  at the cost of possibly causing it to slip and miss steps.
    boardIndex[i]->setAcc(400);
    boardIndex[i]->setDec(400);
  }
}

// Reads back the "config" register from each board in the series.
//  This should be 0x2e88 after a reset of the Autodriver board, or
//  of the control board (as it resets the Autodriver at startup).
//  A reading of 0x0000 means something is wrong with your hardware.
void getBoardConfigurations()
{
  int paramValue;
  Serial.println("Board configurations:");
  for (int i = 0; i < NUM_BOARDS ; i++)
  {
    // It's nice to know if our board is connected okay. We can read
    //  the config register and print the result; it should be 0x2e88
    //  on startup.
    paramValue = boardIndex[i]->getParam(CONFIG);
    Serial.print("Config value: ");
    Serial.println(paramValue, HEX);
  }
}

