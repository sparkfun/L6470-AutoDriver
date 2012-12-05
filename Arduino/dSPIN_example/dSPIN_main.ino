//dSPIN_main.ino - Contains the setup() and loop() functions.

float testSpeed = 25;

void setup() 
{
  // Standard serial port initialization for debugging.
  Serial.begin(9600);
  
  // pin configurations for the test jig
  pinMode(STAT1, OUTPUT);
  pinMode(STAT2, OUTPUT);
  pinMode(SWITCH, INPUT);
  digitalWrite(STAT1, LOW);
  digitalWrite(STAT2, LOW);
  
  // dSPIN_init() is implemented in the dSPIN_support.ino file. It includes
  //  all the necessary port setup and SPI setup to allow the Arduino to
  //  control the dSPIN chip and relies entirely upon the pin redefinitions
  //  in dSPIN_example.ino
  dSPIN_init();
  
  // First things first: let's check communications. The CONFIG register should
  //  power up to 0x2E88, so we can use that to check the communications.
  //  On the test jig, this causes an LED to light up.
  if (dSPIN_GetParam(dSPIN_CONFIG) == 0x2E88) digitalWrite(STAT1, HIGH);
  
  // The following function calls are for this demo application- you will
  //  need to adjust them for your particular application, and you may need
  //  to configure additional registers.
  
  // First, let's set the step mode register:
  //   - dSPIN_SYNC_EN controls whether the BUSY/SYNC pin reflects the step
  //     frequency or the BUSY status of the chip. We want it to be the BUSY
  //     status.
  //   - dSPIN_STEP_SEL_x is the microstepping rate- we'll go full step.
  //   - dSPIN_SYNC_SEL_x is the ratio of (micro)steps to toggles on the
  //     BUSY/SYNC pin (when that pin is used for SYNC). Make it 1:1, despite
  //     not using that pin.
  dSPIN_SetParam(dSPIN_STEP_MODE, !dSPIN_SYNC_EN | dSPIN_STEP_SEL_1 | dSPIN_SYNC_SEL_1);
  // Configure the MAX_SPEED register- this is the maximum number of (micro)steps per
  //  second allowed. You'll want to mess around with your desired application to see
  //  how far you can push it before the motor starts to slip. The ACTUAL parameter
  //  passed to this function is in steps/tick; MaxSpdCalc() will convert a number of
  //  steps/s into an appropriate value for this function. Note that for any move or
  //  goto type function where no speed is specified, this value will be used.
  dSPIN_SetParam(dSPIN_MAX_SPEED, MaxSpdCalc(400));
  // Configure the FS_SPD register- this is the speed at which the driver ceases
  //  microstepping and goes to full stepping. FSCalc() converts a value in steps/s
  //  to a value suitable for this register; to disable full-step switching, you
  //  can pass 0x3FF to this register.
  dSPIN_SetParam(dSPIN_FS_SPD, FSCalc(50));
  // Configure the acceleration rate, in steps/tick/tick. There is also a DEC register;
  //  both of them have a function (AccCalc() and DecCalc() respectively) that convert
  //  from steps/s/s into the appropriate value for the register. Writing ACC to 0xfff
  //  sets the acceleration and deceleration to 'infinite' (or as near as the driver can
  //  manage). If ACC is set to 0xfff, DEC is ignored. To get infinite deceleration
  //  without infinite acceleration, only hard stop will work.
  dSPIN_SetParam(dSPIN_ACC, 0xfff);
  // Configure the overcurrent detection threshold. The constants for this are defined
  //  in the dSPIN_example.ino file.
  dSPIN_SetParam(dSPIN_OCD_TH, dSPIN_OCD_TH_6000mA);
  // Set up the CONFIG register as follows:
  //  PWM frequency divisor = 1
  //  PWM frequency multiplier = 2 (62.5kHz PWM frequency)
  //  Slew rate is 290V/us
  //  Do NOT shut down bridges on overcurrent
  //  Disable motor voltage compensation
  //  Hard stop on switch low
  //  16MHz internal oscillator, nothing on output
  dSPIN_SetParam(dSPIN_CONFIG, 
                   dSPIN_CONFIG_PWM_DIV_1 | dSPIN_CONFIG_PWM_MUL_2 | dSPIN_CONFIG_SR_180V_us
                 | dSPIN_CONFIG_OC_SD_DISABLE | dSPIN_CONFIG_VS_COMP_DISABLE 
                 | dSPIN_CONFIG_SW_HARD_STOP | dSPIN_CONFIG_INT_16MHZ);
  // Configure the RUN KVAL. This defines the duty cycle of the PWM of the bridges
  //  during running. 0xFF means that they are essentially NOT PWMed during run; this
  //  MAY result in more power being dissipated than you actually need for the task.
  //  Setting this value too low may result in failure to turn.
  //  There are ACC, DEC, and HOLD KVAL registers as well; you may need to play with
  //  those values to get acceptable performance for a given application.
  dSPIN_SetParam(dSPIN_KVAL_RUN, 0xFF);
  // Calling GetStatus() clears the UVLO bit in the status register, which is set by
  //  default on power-up. The driver may not run without that bit cleared by this
  //  read operation.
  dSPIN_GetStatus();
}

// Test jig behavior- rotate one full revolution forward, then one full revolution
//  backwards, then slowly tick forwards until the hard stop button is pressed.
void loop()
{
  // 200 steps is one revolution on a 1.8 deg/step motor.
  dSPIN_Move(FWD, 3600);
  while (digitalRead(dSPIN_BUSYN) == LOW);  // Until the movement completes, the
                                            //  BUSYN pin will be low.
  dSPIN_SoftStop();                         // Inserting a soft stop between
                                            //  motions ensures that the driver
                                            //  will execute the next motion with
                                            //  the right speed.
  while (digitalRead(dSPIN_BUSYN) == LOW);  // Wait for the soft stop to complete.
  delay(500);                               // Pause. Not necessary for proper operation.
  dSPIN_Move(REV, 3600);                     // Now do it again, but backwards.
  while (digitalRead(dSPIN_BUSYN) == LOW);
  dSPIN_SoftStop();
  while (digitalRead(dSPIN_BUSYN) == LOW);
  delay(500);
  dSPIN_Run(FWD, SpdCalc(150));       // Now we'll test the hard stop switch...
  while (digitalRead(SWITCH) == HIGH);      // The dSPIN will stop on its own; this is
                                            //  so the Arduino knows the button has been
                                            //  pressed.
  while (digitalRead(dSPIN_BUSYN) == LOW);
  delay(50);
  // Finally check to see if the motor has actually stopped. If so, light the second
  //  LED and wait forever, until the board is removed and the test jig is reset.
  if (dSPIN_GetParam(dSPIN_SPEED) == 0) digitalWrite(STAT2, HIGH);
  while(1);
}
