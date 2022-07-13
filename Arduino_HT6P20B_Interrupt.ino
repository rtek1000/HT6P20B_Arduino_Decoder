/*
  Arduino decoder HT6P20B (using 2M2 resistor in osc)
  - This code (for Arduino) uses timer1 and external interrupt pin.
  - It measures the "pilot period" time (see HT6P20B datasheet for details) and compares if it is within the defined range,
  - If the "pilot period" is within the defined range, it switches to "code period" part (see HT6P20B datasheet for details)
    and takes time measurements at "high level" ("C++" language).
  - If the "code period" is within the previously measured range, add bit 0 or 1 in the _data variable,
    if the "code period" is not within the previously measured range, reset the variables and restart,
  - After receiving all 28 bits, the final 4 bits ("anti-code period", see HT6P20B datasheet for details)
    are tested to verify that the code was received correctly.
  - If received correctly it sets the antcode variable to 1, and the _data2 variable receives the content of the _data variable, without the final 4 bits.
  (Note: For HT6P20A and HT6P20D the code may need to be modified)

  Created by: Jacques Daniel Moresco
  Date: 28/02/2012 using Arduino 0022.
  Allowed public use, but should keep the author’s name.

  ---

  Altered by: Marcelo Lopes Siqueira
  Modifications: Changes made to work with stop using timer attachinterrupt leaving the released for arduino other tasks
  Date 03/07/2013 using Arduino 1.0.5
  Allowed public use, but should keep the authorship.

  Original code: https://forum.arduino.cc/t/decoding-ht6p20-with-attachinterrupt/171483

  ---

  Modified by: Rtek1000
  Modifications:
  - Comments corrected for English language, using Google Translate, and "Text formatting" online (txtformat.com)
  - Added button identification
  - Added "long press" identification
  - Added learning function (Input pin and serial command)
  - Added button outputs (Output pin and serial command)
  - Added learn input pin (Input pin and serial command)
  - Added learn monitoring pin and EEPROM clearing (LED) (Output pin and serial command) 
  - Added clear EEPROM function (Input pin and serial command)
  Date 13/07/2022 using Arduino 1.8.19
  Allowed public use, but should keep the authorship.
*/
#include <EEPROM.h>

#define OUTPUT_ON 1
#define LEARN_INPUT_ON 1
#define SERIAL_ON 1
// #define DEBUG_ON 1

volatile int startbit, startbit1, ctr, dataok, pulse_width, pulse_width1, pulse_width2, pulse_width3, pulse_width4, antcode = 0;
// _data: the code stored from the HT6P20B, all 28 bits, where: 20 bits to "address code" + 4 bits to "data code" + 4 bits to "anti-code".
// _data2: the code stored from the HT6P20B, only 24 bits, where: 20 bits to "address code" + 4 bits to "data code"
volatile unsigned long _data, _data2 = 0;
volatile unsigned long _width, _width1; // Pulse width

#define PPM_Pin 2 // Pin that will receive the digital RF signal. This must be 2 or 3 for UNO (ATmega328p), interrupt pin 0 or 1

#define Out_1_pin 4       // Output for relay or LED indicator, button 1. (Need to use driver) 
#define Out_2_pin 5       // Output for relay or LED indicator, button 2. (Need to use driver) 
#define Out_3_pin 6       // Output for relay or LED indicator, button 3. (Need to use driver) 
#define Out_long_pin 7    // Output for relay or LED indicator, long press. (Need to use driver) 
#define Learn_pin 8       // Input for learn function / clear function (erase all). Input with low state activation (Pull-Up)
#define LED_learn_pin 13  // Output for relay or LED indicator, learn function / clear function (erase all) / receiver function. (Need to use driver) 

bool Learn_pin_old = true;

volatile bool _button1 = LOW;
volatile bool _button2 = LOW;
volatile bool _button3 = LOW;
volatile byte _buttons = 0;
volatile bool _button_flag = LOW;
volatile bool _button_flag_old = LOW;
volatile bool _button_long_flag = LOW;

volatile unsigned long _millis1 = 0;
volatile unsigned long _millis2 = 0;
unsigned long _millis3 = 0;
unsigned long _millis4 = 0;

String inputString = "";
byte inputString_cnt = 0;

bool verbose_mode = false;

void setup() {
  delay(500);

#ifdef OUTPUT_ON
  pinMode(Out_1_pin, OUTPUT);
  pinMode(Out_2_pin, OUTPUT);
  pinMode(Out_3_pin, OUTPUT);
  pinMode(Out_long_pin, OUTPUT);

  digitalWrite(Out_1_pin, LOW);
  digitalWrite(Out_2_pin, LOW);
  digitalWrite(Out_3_pin, LOW);
  digitalWrite(Out_long_pin, LOW);
#endif

#ifdef LEARN_INPUT_ON
  pinMode(Learn_pin, INPUT_PULLUP);
  pinMode(LED_learn_pin, OUTPUT);

  digitalWrite(LED_learn_pin, LOW);
#endif

#ifdef SERIAL_ON
  Serial.begin(115200);
  Serial.println(F("Start"));
#endif

  pinMode(PPM_Pin, INPUT);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  attachInterrupt(digitalPinToInterrupt(PPM_Pin), read_ppm, CHANGE);

  TCCR1A = 0; // Reset timer1
  TCCR1B = 0;
  TCCR1B |= (1 << CS11); // Set timer1 to increment every 0,5 us
}

void loop() {
  if (_button_flag == true) {
    if (check_data() == true) {
#ifdef OUTPUT_ON
      digitalWrite(Out_1_pin, _button1);
      digitalWrite(Out_2_pin, _button2);
      digitalWrite(Out_3_pin, _button3);

      digitalWrite(LED_learn_pin, HIGH);
#endif

      if (_button_long_flag == false) {
#ifdef SERIAL_ON
        if (verbose_mode == true) {
          if (_button1 == true) {
            Serial.println(F("Button 1 pressed"));
          } else if (_button2 == true) {
            Serial.println(F("Button 2 pressed"));
          } else if (_button3 == true) {
            Serial.println(F("Button 3 pressed"));
          }
        } else {
          if (_button1 == true) {
            Serial.println(F("1"));
          } else if (_button2 == true) {
            Serial.println(F("2"));
          } else if (_button3 == true) {
            Serial.println(F("3"));
          }
        }
#endif
#ifdef OUTPUT_ON
        digitalWrite(Out_long_pin, LOW);
#endif
      } else {
#ifdef SERIAL_ON
        if (verbose_mode == true) {
          if (_button1 == true) {
            Serial.println(F("Button 1 long press"));
          } else if (_button2 == true) {
            Serial.println(F("Button 2 long press"));
          } else if (_button3 == true) {
            Serial.println(F("Button 3 long press"));
          }
        } else {
          if (_button1 == true) {
            Serial.println(F("4"));
          } else if (_button2 == true) {
            Serial.println(F("5"));
          } else if (_button3 == true) {
            Serial.println(F("6"));
          }
        }
#endif
#ifdef OUTPUT_ON
        digitalWrite(Out_long_pin, HIGH);
#endif
      }
    } else {
#ifdef SERIAL_ON
      if (verbose_mode == true) {
        Serial.println(F("Code not registered"));
      } else {
        Serial.println(F("U")); // unknown
      }
#endif
    }

    _millis3 = millis();

    _button_flag = false;
  } else {
    if (millis() > (_millis3 + 150)) {
      _millis3 = millis();
#ifdef OUTPUT_ON
      digitalWrite(Out_1_pin, LOW);
      digitalWrite(Out_2_pin, LOW);
      digitalWrite(Out_3_pin, LOW);

      digitalWrite(Out_long_pin, LOW);

      digitalWrite(LED_learn_pin, LOW);
#endif
#ifdef SERIAL_ON
      if (verbose_mode == false) {
        Serial.println(F("0"));
      }
#endif
    }
  }

#ifdef LEARN_INPUT_ON
  if ((digitalRead(Learn_pin) == LOW) && (Learn_pin_old == HIGH)) {
    _millis4 = millis();

    Learn_pin_old = LOW;

    while ((digitalRead(Learn_pin) == LOW) && (millis() < (_millis4 + 5000))) {
      digitalWrite(LED_learn_pin, !digitalRead(LED_learn_pin));
      delay(100);
    }

    if (millis() >= (_millis4 + 5000)) {
      _millis4 = millis();

      while ((digitalRead(Learn_pin) == LOW) && (millis() < (_millis4 + 5000))) {
        digitalWrite(LED_learn_pin, !digitalRead(LED_learn_pin));
        delay(50);
      }

      if (millis() < (_millis4 + 3000)) {
        clear_func();

        digitalWrite(LED_learn_pin, HIGH);

        delay(1000);
      }
    } else if (millis() > (_millis4 + 250)) {
      learn_func();
    }

    digitalWrite(LED_learn_pin, LOW);
  } else if (digitalRead(Learn_pin) == HIGH) {
    Learn_pin_old = HIGH;
  }
#endif
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
#ifdef SERIAL_ON
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    inputString_cnt++;

    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if ((inChar == '\r') || (inChar == '\n')) {
      if ((inputString == "L") || (inputString == "l")) {
        learn_func();
      } else if ((inputString == "C") || (inputString == "c")) {
        clear_func();
      } else if ((inputString == "V") || (inputString == "v")) {
        verbose_mode = !verbose_mode;

        if (verbose_mode == true) {
          Serial.println(F("Verbose Mode On"));
        } else {
          Serial.println(F("Verbose Mode Off"));
        }
      }

      inputString_cnt = 0;
      inputString = "";
      inChar = 0;

      while (Serial.available()) {
        Serial.read(); // Clear buffer
      }
    } else {
      if (inputString_cnt < 2) {
        // add it to the inputString:
        inputString += inChar;
      } else {
        inputString_cnt = 0;
        inputString = "";
      }
    }
  }
}
#endif

void learn_func() {
  unsigned long millis_learn_timeout = 0;

  Serial.println(F("Learn"));
  Serial.println(F("Press a buttun now"));

  millis_learn_timeout = millis();

  byte cnt_timeout = 5;

  while ((_button_flag == false) && (cnt_timeout > 0)) {
    Serial.print(F("Time count: "));
    Serial.print(cnt_timeout, DEC);
    Serial.println(F("s"));
    cnt_timeout--;

    digitalWrite(LED_learn_pin, LOW);
    delay(500);

    digitalWrite(LED_learn_pin, HIGH);
    delay(500);
  }

  if (_button_flag == true) {
    int res_save_data = save_data();

    if (res_save_data == 1) {
      Serial.println(F("This code is already saved"));
      digitalWrite(LED_learn_pin, HIGH);
      delay(250);
      digitalWrite(LED_learn_pin, LOW);
      delay(500);
    } else if (res_save_data == 2) {
      Serial.println(F("Code is saved successfully"));
      digitalWrite(LED_learn_pin, HIGH);
      delay(250);
      digitalWrite(LED_learn_pin, LOW);
      delay(500);
      digitalWrite(LED_learn_pin, HIGH);
      delay(250);
      digitalWrite(LED_learn_pin, LOW);
      delay(500);
    } else if (res_save_data == -1) {
      Serial.println(F("Learn ERROR: no more EEPROM space"));
      digitalWrite(LED_learn_pin, HIGH);
      delay(500);
      digitalWrite(LED_learn_pin, LOW);
      delay(500);
    } else {
      Serial.println(F("Learn ERROR: invalid data"));
    }

    _button_flag = false;

    digitalWrite(LED_learn_pin, LOW);
  } else {
    Serial.println(F("Learn ERROR: timeout"));
  }
}

void clear_func() {
  Serial.println(F("Clear"));

  if (clear_EEPROM() == true) {
    Serial.println(F("Clear OK"));
  } else {
    Serial.println(F("Clear ERROR"));
  }
}

bool check_data() {
  _data2 = _data2 >> 4; // 24 bits to 20 bits ("address code" only)

  int base_address = 0;

  byte _data0 = _data2 & 0xFF;         // A0 to A7 bits
  byte _data1 = (_data2 >> 8) & 0xFF;  // A8 to A15 bits
  byte _data2 = (_data2 >> 16) & 0x0F; // A15 to A19 bits

  while (E2END > (base_address + 3)) { // E2END last addr of EEPROM
    if ((EEPROM.read(base_address) == _data0) and
        (EEPROM.read(base_address + 1) == _data1) and
        (EEPROM.read(base_address + 2) == _data2)) {
      return true; // found
    } else {
      if (E2END < (base_address + 3)) {
        return false; // no more EEPROM space
      } else {
        base_address += 3; // try again
      }
    }
  }

  return false; // not found
}

int save_data() {
  _data2 = _data2 >> 4; // 24 bits to 20 bits ("address code" only)

  int base_address = 0;

  byte _data0 = _data2 & 0xFF;         // A0 to A7 bits
  byte _data1 = (_data2 >> 8) & 0xFF;  // A8 to A15 bits
  byte _data2 = (_data2 >> 16) & 0x0F; // A15 to A19 bits

  while (E2END > (base_address + 3)) { // E2END last addr of EEPROM
    if ((EEPROM.read(base_address) == _data0) and
        (EEPROM.read(base_address + 1) == _data1) and
        (EEPROM.read(base_address + 2) == _data2)) {
      return 1; // This code is already saved
    } else {
      if ((EEPROM.read(base_address) == 0xFF) and
          (EEPROM.read(base_address + 1) == 0xFF) and
          (EEPROM.read(base_address + 2) == 0xFF)) {
        EEPROM.update(base_address, _data0);
        EEPROM.update(base_address + 1, _data1);
        EEPROM.update(base_address + 2, _data2);

        return 2; // Code is saved successfully
      } else {
        if (E2END < (base_address + 3)) {
          return -1; // no more EEPROM space
        } else {
          base_address += 3; // try again
        }
      }
    }
  }

  return 0;
}

bool clear_EEPROM() {
  int base_address = 0;

  while (E2END > base_address) { // E2END last addr of EEPROM
    EEPROM.update(base_address, 0xFF);

    base_address++;
  }

  base_address = 0;

  while (E2END > base_address) { // E2END last addr of EEPROM
    if (EEPROM.read(base_address) != 0xFF) {
      return false;
    }

    base_address++;
  }

  return true;
}

void read_ppm() { // leave this alone
  static unsigned long counter;

  // 1020 cycles equal a 510us pulse
  // 2000 1000us
  // 20000 10000us
  // x 12000

  counter = TCNT1; // Add the number of cycles
  TCNT1 = 0;

  _width = counter;

  // Test pilot time to start bit;
  if (_width > 20000 && _width < 24000 && startbit == 0) {
    pulse_width = _width / 23;
    pulse_width1 = pulse_width - 150;
    pulse_width2 = pulse_width + 150;
    pulse_width3 = pulse_width + pulse_width - 150;
    pulse_width4 = pulse_width + pulse_width + 150;
    startbit = 1;
    _width = 0;
    _data = 0;
    dataok = 0;
    ctr = 0;

    if (millis() > (_millis2 + 250)) {
      _millis1 = millis();
      _button_long_flag = false;
    }
  } // End of pilot time condition (beginning of bits)

  if (startbit == 1) {
    startbit++;
  } else if (startbit == 2 && dataok == 0 && ctr < 28 && digitalRead(2) == HIGH) { // If it has a valid start bit
    ++ctr;
    _width1 = counter;

    if (_width1 > pulse_width1 && _width1 < pulse_width2) { // If the pulse width is between 1/4000 and 1/3000 second
      _data = (_data << 1) ; // Append a *1* to the rightmost end of the buffer
    } else if (_width1 > pulse_width3 && _width1 < pulse_width4) { // If the pulse width is between 2/4000 and 2/3000 seconds
      _data = (_data << 1) + 1; // Append a *0* to the rightmost end of the buffer
    } else {
      startbit = 0; // Force loop termination
    }
  }

  if (ctr == 28) {
    if (bitRead(_data, 0) == 1 && bitRead(_data, 1) == 0 && bitRead(_data, 2) == 1 && bitRead(_data, 3) == 0) {
      antcode = 1;
    } else {
      ctr = 0;
    }

    if (antcode == 1) { // If all 28 bits were received, the value goes to the _data variable and can be used as an example below.
      dataok = 1;

#ifdef SERIAL_ON
#ifdef DEBUG_ON
      Serial.print(F("Antecode OK - ") + String(_data, BIN));
#endif
#endif

      ctr = 0;
      startbit = 0;
      antcode = 0;
      _data2 = _data >> 4; // 28 bits to 24 bits ("address code" + "data code" only)

      byte _data3 = _data2 & 0x03; // Filter for model HT6P20B, only 2 bits for buttons. 

      _buttons = _data3;

      _button1 = false;
      _button2 = false;
      _button3 = false;

      if (_data3 == 2) {
        _button1 = true;
      } else if (_data3 == 1) {
        _button2 = true;
      } else if (_data3 == 3) {
        _button3 = true;
      }

      _button_flag = true;

      _millis2 = millis();

      if (_button_flag_old == _button_flag) {
        if (millis() > (_millis1 + 500)) {
          _button_long_flag = true;
        }
      } else {
        _millis1 = millis();
        _button_flag_old = _button_flag;
        _button_long_flag = false;
      }
    }
  }

}// End of read_ppm() function
