//dSPIN_main.ino - Contains the setup() and loop() functions.

float testSpeed = 10;

void setup() 
{
  // Standard serial port initialization for debugging.
  Serial.begin(9600);
    
  // dSPIN_init() is implemented in the dSPIN_support.ino file. It includes
  //  all the necessary port setup and SPI setup to allow the Arduino to
  //  control the dSPIN chip and relies entirely upon the pin redefinitions
  //  in dSPIN_example.ino
  dSPIN_init();
  
  init_dSPIN(0);
  init_dSPIN(1);
  init_dSPIN(2);
 
}

// Test jig behavior- rotate one full revolution forward, then one full revolution
//  backwards, then slowly tick forwards until the hard stop button is pressed.
void loop()
{
  // 200 steps is one revolution on a 1.8 deg/step motor.
  dSPIN_Move(FWD, 400,0);
  dSPIN_Move(FWD, 400,1);
  dSPIN_Move(FWD, 400,2);
  while (digitalRead(dSPIN_BUSYN) == LOW);  // Until the movement completes, the
                                            //  BUSYN pin will be low.
  dSPIN_SoftStop(0);
  dSPIN_SoftStop(1);
  dSPIN_SoftStop(2);
  
  delay(500);
  
  dSPIN_Run(FWD, 500, 0);
  dSPIN_Run(FWD, 500, 1);
  dSPIN_Run(FWD, 500, 2);
  delay(2500);
  dSPIN_SoftStop(0);
  dSPIN_SoftStop(1);
  dSPIN_SoftStop(2);
  
  while(1);
}

// Chip inits for all the chips.
void init_dSPIN(byte chip)
{
  // First things first: let's check communications. The CONFIG register should
  //  power up to 0x2E88, so we can use that to check the communications.
  //  On the test jig, this causes an LED to light up.
  if (dSPIN_GetParam(dSPIN_CONFIG,chip) == 0x2E88) 
  {
    Serial.print("Chip ");
    Serial.print(chip);
    Serial.println(" okay!");
  }
  
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
  dSPIN_SetParam(dSPIN_STEP_MODE, !dSPIN_SYNC_EN | dSPIN_STEP_SEL_1 | dSPIN_SYNC_SEL_1,chip);
  // Configure the MAX_SPEED register- this is the maximum number of (micro)steps per
  //  second allowed. You'll want to mess around with your desired application to see
  //  how far you can push it before the motor starts to slip. The ACTUAL parameter
  //  passed to this function is in steps/tick; MaxSpdCalc() will convert a number of
  //  steps/s into an appropriate value for this function. Note that for any move or
  //  goto type function where no speed is specified, this value will be used.
  dSPIN_SetParam(dSPIN_MAX_SPEED, MaxSpdCalc(400),chip);
  // Configure the FS_SPD register- this is the speed at which the driver ceases
  //  microstepping and goes to full stepping. FSCalc() converts a value in steps/s
  //  to a value suitable for this register; to disable full-step switching, you
  //  can pass 0x3FF to this register.
  dSPIN_SetParam(dSPIN_FS_SPD, FSCalc(400),chip);
  // Configure the acceleration rate, in steps/tick/tick. There is also a DEC register;
  //  both of them have a function (AccCalc() and DecCalc() respectively) that convert
  //  from steps/s/s into the appropriate value for the register. Writing ACC to 0xfff
  //  sets the acceleration and deceleration to 'infinite' (or as near as the driver can
  //  manage). If ACC is set to 0xfff, DEC is ignored. To get infinite deceleration
  //  without infinite acceleration, only hard stop will work.
  dSPIN_SetParam(dSPIN_ACC, 0xfff,chip);
  // Configure the overcurrent detection threshold. The constants for this are defined
  //  in the dSPIN_example.ino file.
  dSPIN_SetParam(dSPIN_OCD_TH, dSPIN_OCD_TH_6000mA,chip);
  // Set up the CONFIG register as follows:
  //  PWM frequency divisor = 1
  //  PWM frequency multiplier = 2 (62.5kHz PWM frequency)
  //  Slew rate is 290V/us
  //  Do NOT shut down bridges on overcurrent
  //  Disable motor voltage compensation
  //  Hard stop on switch low
  //  16MHz internal oscillator, 16MHz out
  dSPIN_SetParam(dSPIN_CONFIG, 
                   dSPIN_CONFIG_PWM_DIV_1 | dSPIN_CONFIG_PWM_MUL_2 | dSPIN_CONFIG_SR_290V_us
                 | dSPIN_CONFIG_OC_SD_DISABLE | dSPIN_CONFIG_VS_COMP_DISABLE 
                 | dSPIN_CONFIG_SW_HARD_STOP | dSPIN_CONFIG_INT_16MHZ_OSCOUT_16MHZ,chip);
  // We're going to change this a bit, now- we want to use the clock output from the first
  //  chip to drive the second chip, and the output from the second chip to drive the third.
  //  That means setting up chips two and three to use an external 16MHz clock and to dump
  //  that clock signal to the clock output pin. Otherwise, settings are the same as above.
  if (chip > 0)
  {
  dSPIN_SetParam(dSPIN_CONFIG, 
                   dSPIN_CONFIG_PWM_DIV_1 | dSPIN_CONFIG_PWM_MUL_2 | dSPIN_CONFIG_SR_290V_us
                 | dSPIN_CONFIG_OC_SD_DISABLE | dSPIN_CONFIG_VS_COMP_DISABLE 
                 | dSPIN_CONFIG_SW_HARD_STOP | dSPIN_CONFIG_EXT_16MHZ_OSCOUT_INVERT,chip);
  }
    
  // Configure the RUN KVAL. This defines the duty cycle of the PWM of the bridges
  //  during running. 0xFF means that they are essentially NOT PWMed during run; this
  //  MAY result in more power being dissipated than you actually need for the task.
  //  Setting this value too low may result in failure to turn.
  //  There are ACC, DEC, and HOLD KVAL registers as well; you may need to play with
  //  those values to get acceptable performance for a given application.
  dSPIN_SetParam(dSPIN_KVAL_RUN, 0xFF,chip);
  dSPIN_SetParam(dSPIN_KVAL_ACC, 0x7F,chip);
  // Calling GetStatus() clears the UVLO bit in the status register, which is set by
  //  default on power-up. The driver may not run without that bit cleared by this
  //  read operation.
  dSPIN_GetStatus(chip);
}
