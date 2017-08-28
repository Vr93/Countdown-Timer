/*
  Count down timer.
  Arduino Nano, Rotary Encoder with button and 0.96" LCD Display.
  Main menu with the possibility to easily add more options, other than
  the ones I have made.
    - Made By Vegard Rogne.
    ** Version 1.0 **
*/
/*
  Wiring Diagram.
                                   ARDUINO NANO
                                  | TX    Vin |------ External Power Supply(
                                  | RX    GND |------ GND(LCD),GND(Rotary Enc).
                                  | RST   RST |
                                  | GND   5V  |------ 5V(LCD),5V(Rotary Enc),4.7kΩ Pullup resistor(A4),4.7kΩ Pullup resistor(A5)
         CLK(Rotary Encoder)------| D2    A7  |
          DT(Rotary Encoder)------| D3    A6  |
          SW(Rotary Encoder)------| D4    A5  |------ SDA(LCD)
                                  | D5    A4  |------ SCL(LCD)
                                  | D6    A3  |
                                  | D7    A2  |
                                  | D8    A1  |
                                  | D9    A0  |
                                  | D10   REF |
                                  | D11   3V3 |
                                  | D12   D13 |

  Please pay attention to the pullup resistor from 5V to SDA And SCL pins!

*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>


/*
   Encoder Variables.
*/
Encoder myEnc(3, 2);
int rotaryBtn = 4;
unsigned long rotatingMillis = 0;
long encoderLastState = 0;
/*
   End of Encoder Variables.
*/
/*
   0.96" OLED LCD Display Variables
*/
int logoValue = 0;
unsigned long logoMillis = 0;
Adafruit_SSD1306 display(4);
int displayValue = 1;
boolean editingHourValue = false;
boolean editingMinutesValue = false;
String minutesValue = "00";
String hoursValue = "00";
boolean startTimer = false;
boolean lastButtonState = false;
int lastButtonStateRelease = 0;
boolean idleStateSet = false;
/*
   End of 0.96" OLED LCD Display Variables
*/
/*
   Timer Variables.
*/
unsigned long blinkTimerMillis = 0;
unsigned long timerCountDownMillis = 0;
unsigned long buttonStateMillis = 0;
unsigned long changeTimeVariableMillis = 0;

/*
 * If you want minutes and seconds, instead of
 * hours and minutes, change countDownMillis
 * variable to 1000.
 */
unsigned long countDownMillis = 60000;

/*
   End of Timer Variables.
*/

void setup() {
  pinMode(rotaryBtn, INPUT_PULLUP);
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);
}

void loop() {
  lcdLogic();
}

/*
   LCD Logic contains all logic about
   menu selections etc...
   0 - Logo
   1 - Main Menu
   2 - Timer
   3 - Reserved
*/
void lcdLogic() {
  /*
     Main Menu, has several options scrollable
     with encoder and enterable by pushing the button.
  */

  /*
     0 - Logo.
     Checks the rotary encoder for activity,
     if none the lcd shows the logo.
  */
  if ((displayValue == 0) && (idleStateSet)) {
    showLogoLCD();
  }
  else if ((!idleStateSet) && (displayValue < 1)) {
    displayValue = 1;
  }
  /*
     1 - Main Menu.
     The meny currently has two options, timer and reserved.
     Scrollable by using the rotary encoder and activates the
     desired option by clicking the button.
  */
  if (displayValue == 1) {
    int value = readRotaryEncoder();
    if (value < 0) {
      resetEncoder(2);
      value = 2;
    }
    if (value > 2) {
      resetEncoder(1);
      value = 1;
    }
    showMainMenuLCD(value);
    /*
       If Button Is Pressed, Go From Main Menu To The Selected Option.
       Timer Has Been Added To Avoid Returning To The Same Menu When
       Going Back To Main Menu.
    */
    if (readPressReleaseButton()) {
      /*
         To Timer Menu.
      */
      if (value == 1) {
        displayValue = 2;
        resetEncoder(0);
      }
      /*
         To reserved Menu.
      */
      if (value == 2) {
        Serial.println("To reserved Menu"); // Code Missing... You do the rest! :D
      }
    }
  }
  /*
       Timer Menu, has several options scrollable
       with encoder and enterable by pushing the button.
  */
  if (displayValue == 2) {
    int  value = readRotaryEncoder();
    if ((!editingHourValue) && (!editingMinutesValue)) {
      /*
         To avoid going further than menu selection.
      */
      if (value < 1) {
        value = 1;
        resetEncoder(1);
      }
      /*
         To avoid going further than menu selection.
      */
      if (value > 4) {
        value = 4;
        resetEncoder(4);
      }
      /*
         Update 0.96" OLED LCD with the selected value.
      */
      showTimerMenuLCD(value);
    }
    /*
       If button is pressed, set various states.
    */
    if (readPressReleaseButton()) {
      /*
        1 - Change Hours.
        Allowed to change value if the
        timer has not started.
      */
      if (value == 1) {
        if (!startTimer) {
          editingHourValue = true;
          resetEncoder(0);
          changeTimeVariableMillis = millis();
        }
      }
      /*
         2 - Change Minutes.
         Allowed to change value if the
         timer has not started.
      */
      if (value == 2) {
        if (!startTimer) {
          editingMinutesValue = true;
          resetEncoder(0);
          changeTimeVariableMillis = millis();
        }
      }
      /*
         3 - Back To Main Menu.
      */
      if (value == 3) {
        displayValue = 1;
      }
      /*
         4 - Start Timer.
      */
      if (value == 4) {
        if (!startTimer) {
          startTimer = true;
        }
        else {
          startTimer = false;
        }
      }
    }

    /*
      To Update LCD When Editing Hour Variable.
    */
    if (editingHourValue) {
      showTimerMenuLCD(0);
    }
    /*
      To Update LCD When Editing Minute Variable.
    */
    if (editingMinutesValue) {
      showTimerMenuLCD(0);
    }
  }
  /*
     Checks the rotary encoder for activity,
     if none set displayValue to zero (Logo),
     and another boolean value true.
  */
  boolean idle = checkEncoderIdleState();
  if ((idle) && (!startTimer)) {
    displayValue = 0;
    idleStateSet = true;
  }
  else {
    idleStateSet = false;
  }
  timerLogic();
}

/*
   Resets The Encoder To Parameter Value.
   It is multiplied by 4 because the read
   function divides the value by 4 to get accurate
   readings!
*/
void resetEncoder(int value) {
  encoderLastState = value;
  myEnc.write(value * 4);
}

/*
   Timer Logic, Awaits Timer State
   To Start.
*/
void timerLogic() {
  /*
     Starts timer count down with values entered by the user.
     The minute value counts down and increments the countDownMillis 
     until both are zero.
  */
  unsigned long currentMillis = millis();
  if (startTimer) {
    if (currentMillis - timerCountDownMillis > countDownMillis) {
      minutesValue = minutesValue.toInt() - 1;
      timerCountDownMillis = currentMillis;
      if (hoursValue.toInt() == 0) {
        hoursValue = "00";
      }
      if ((hoursValue == "00") && (minutesValue.toInt() < 0)) {
        minutesValue = "00";
        startTimer = false;
      }
      else if (minutesValue.toInt() < 0) {
        minutesValue = 59;
        hoursValue = hoursValue.toInt() - 1;
      }
    }
  }
}

/*
   Reads Encoder The State.
*/
long readRotaryEncoder() {
  return myEnc.read() / 4;
}



/*
   Reads the encoder button,
   returns true if pushed.
*/
boolean readRotaryButton() {
  return !digitalRead(rotaryBtn);
}

boolean readPressReleaseButton() {
  boolean returnValue = false;
  unsigned long currentMillis = millis();
  if ((readRotaryButton() != lastButtonState) && (currentMillis - buttonStateMillis > 500)) {
    returnValue = true;
    buttonStateMillis = currentMillis;
  }
  lastButtonState = readRotaryButton();
  return returnValue;
}

void showTimerMenuLCD(int value) {
  display.clearDisplay();
  unsigned long currentMillis = millis();
  /*
     Swich case, for different options within the timer menu.
     0 - Empty, DO NOTHING.
     1 - Hours Variable.
     2 - Minute variable.
     3 - Back To Main Menu.
     4 - Start Timer.
  */
  switch (value) {
    case 0:
      // DO NOTHING..
      break;
    case 1:
      display.setCursor(20, 17);
      display.print("______");
      break;
    case 2:
      display.setCursor(73, 17);
      display.print("______");
      break;
    case 3:
      display.setCursor(0, 25);
      display.print("**");
      display.setCursor(35, 25);
      display.print("**");
      break;
    case 4:
      if (!startTimer) {
        display.setCursor(37, 25);
        display.print("**");
        display.setCursor(115, 25);
        display.print("**");
      }
      else {
        display.setCursor(42, 25);
        display.print("**");
        display.setCursor(115, 25);
        display.print("**");
      }
      break;
    case 5:
      break;
  }
  /*
     If Not Updating Hours Variable, Display Value
     Accordingly To Its Number For Visual Perfection!
  */
  if (!editingHourValue) {
    display.setTextSize(3);
    int tempHoursValue = hoursValue.toInt();
    if ((tempHoursValue < 10) && (tempHoursValue >= 0)) {
      display.setCursor(20, 0);
      display.print(0);
      display.setCursor(40, 0);
      display.print(tempHoursValue);
    }
    else if ((tempHoursValue > 99) && (tempHoursValue < 1000)) {
      display.setCursor(5, 0);
      display.print(tempHoursValue);
    }
    else {
      display.setCursor(20, 0);
      display.print(tempHoursValue);
    }
  }
  /*
     Updating Hours Variable, check for various states
     and correct visual positions(LCD).
  */
  else {
    if (currentMillis - blinkTimerMillis > 1000) {
      blinkTimerMillis = currentMillis;
    }
    else {
      display.setTextSize(3);
      if (readRotaryEncoder() < 0) {
        resetEncoder(999);
        display.setCursor(5, 0);
        display.print(readRotaryEncoder());
      }
      else if ((readRotaryEncoder() < 10) && (readRotaryEncoder() >= 0)) {
        display.setCursor(20, 0);
        display.print(0);
        display.setCursor(40, 0);
        display.print(readRotaryEncoder());
      }
      else if ((readRotaryEncoder() > 99) && (readRotaryEncoder() < 1000)) {
        display.setCursor(5, 0);
        display.print(readRotaryEncoder());
      }
      else if (readRotaryEncoder() > 999) {
        resetEncoder(0);
        display.setCursor(20, 0);
        display.print(0);
        display.setCursor(40, 0);
        display.print(readRotaryEncoder());
      }
      else {
        display.setCursor(20, 0);
        display.print(readRotaryEncoder());
      }
      /*
         Check for push button when correct value has been entered, to
         set hour variable.
      */
      if ((readRotaryButton()) && (millis() - changeTimeVariableMillis > 1000)) {
        hoursValue = readRotaryEncoder();
        editingHourValue = false;
        resetEncoder(2);
      }
    }
  }
  /*
    If Not Updating Minutes Variable, Display Value
    Accordingly To Its Number For Visual Perfection!
  */
  if (!editingMinutesValue) {
    display.setTextSize(3);
    if ((minutesValue.toInt() < 10) && (minutesValue != "00")) {
      display.setCursor(75, 0);
      display.print(0);
      display.setCursor(95, 0);
      display.print(minutesValue);
    }
    else {
      display.setCursor(75, 0);
      display.print(minutesValue);
    }
  }
  /*
    Updating Minutes Variable, check for various states
    and correct visual positions.
  */
  else {
    if (currentMillis - blinkTimerMillis > 1000) {
      blinkTimerMillis = currentMillis;
    }
    else {
      display.setTextSize(3);
      if (readRotaryEncoder() > 59) {
        resetEncoder(0);
      }
      else if (readRotaryEncoder() < 0) {
        resetEncoder(59);
      }
      if (readRotaryEncoder() < 10) {
        display.setCursor(75, 0);
        display.print(0);
        display.setCursor(95, 0);
        display.print(readRotaryEncoder());
      }
      else {
        display.setCursor(75, 0);
        display.print(readRotaryEncoder());
      }
      /*
         Check for push button when correct value has been entered, to
         set Minutes variable.
      */
      if ((readRotaryButton()) && (millis() - changeTimeVariableMillis > 1000)) {
        minutesValue = readRotaryEncoder();
        editingMinutesValue = false;
        resetEncoder(4);
      }
    }
  }
  /*
    Display the remaining optinos in timer menu.
  */
  display.setTextSize(3);
  display.setCursor(57, 0);
  display.print(":");
  display.setTextSize(1);
  display.setCursor(12, 25);
  display.print("Back");
  if (!startTimer) {
    display.setCursor(50, 25);
    display.print("Start Timer");
  }
  else {
    display.setCursor(55, 25);
    display.print("Stop Timer");
  }
  display.display();
}


void showMainMenuLCD(int value) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(55, 0);
  display.print("Menu");
  display.setCursor(0, 2);
  display.print("_____________________");
  display.setCursor(20, 10);
  display.print("Countdown Timer");
  display.setCursor(33, 20);
  display.print("Reserved");
  switch (value) {
    case 0:
      // DO NOTHING
      break;
    case 1:
      display.setCursor(0, 10);
      display.print("**");
      display.setCursor(115, 10);
      display.print("**");
      break;
    case 2:
      display.setCursor(0, 20);
      display.print("**");
      display.setCursor(115, 20);
      display.print("**");
      break;
  }

  display.display();
}

void showLogoLCD() {
  if ((logoValue < 35) && (millis() - logoMillis > 100)) {
    display.clearDisplay();
    display.setCursor(30, 7 + logoValue);
    display.print("Vegard");
    display.setCursor(37, 22 + logoValue);
    display.print("Rogne");
    if (logoValue > 5) {
      display.setCursor(37, logoValue - 18);
      display.print("Rogne");
      display.setCursor(30, logoValue - 33);
      display.print("Vegard");
    }
    logoValue++;
    logoMillis = millis();
    display.setTextSize(2);
    display.display();
  }
  else if ((logoValue >= 35) && (millis() - logoMillis > 5000)) {
    logoValue = 0;
  }
}

/*
   Checks all states of the rotary encoder,
   to check if it has been activated within the given time.
   If not, returns true indicating idle state.
*/
boolean checkEncoderIdleState() {
  boolean encoderInactive = false;
  /*
     Counter for idle state, if over the given value
     returns true.
  */
  if (millis() - rotatingMillis > 60000) {
    encoderInactive = true;
  }
  /*
     Checks button state, if activated
     reset timer.
  */
  if (readRotaryButton()) {
    rotatingMillis = millis();
  }
  if (readPressReleaseButton()) {
    rotatingMillis = millis();
  }
  /*
     Checks encoder state, if activated
     reset timer.
  */
  if (readRotaryEncoder() != encoderLastState) {
    encoderLastState = readRotaryEncoder();
    rotatingMillis = millis();
  }
  return encoderInactive;
}






