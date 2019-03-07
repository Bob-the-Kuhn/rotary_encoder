/**
 * 
 * Copyright (C) 2019 Bob Kuhn bob.kuhn@att.net
 *
 * Based on Sprinter and grbl.
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For the GNU General Public License see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * Program to display rotary encoder counts
 *
 * SELECT button cycles through the axis labels available.
 * UP button resets the count in the top line.
 * DOWN button resets the count in the bottom line.
 *
 * Hardware: 
 *   HCTL_2032 dual quadrature decoder/counter
 *   Teensy++2.0 controller
 *   LCD 1602 2x16 display with keypad
 *
 * LCD & keypad code from example by Mark Bramwell, July 2010
 * 
 * 
 */

#include <Arduino.h>
#include <LiquidCrystal.h>


//
// HCTL-2032
//

#define HCTL_2032_D0       0 // port D bit 0 
#define HCTL_2032_D1       1 // port D bit 1
#define HCTL_2032_D2       2 // port D bit 2
#define HCTL_2032_D3       3 // port D bit 3
#define HCTL_2032_D4       4 // port D bit 4
#define HCTL_2032_D5       5 // port D bit 5
#define HCTL_2032_D6       6 // port D bit 6
#define HCTL_2032_D7       7 // port D bit 7
#define HCTL_2032_SEL1    10 // port C bit 0
#define HCTL_2032_SEL2    11 // port C bit 1
#define HCTL_2032_EN1     12 // port C bit 2
#define HCTL_2032_EN2     13 // port C bit 3
#define HCTL_2032_OE      14 // port C bit 4
#define HCTL_2032_CLK     15 // port C bit 5
#define HCTL_2032_RSTX    16 // port C bit 6
#define HCTL_2032_RSTY    17 // port C bit 7
#define HCTL_2032_XY      18//17//18 // port E bit 6
#define HCTL_2032_DATA_PORT  PORTD

#define ENABLE_OUTPUT  digitalWrite(HCTL_2032_OE, LOW)
#define DISABLE_OUTPUT digitalWrite(HCTL_2032_OE, HIGH)

#define ADD_BYTE_AXIS_A do {count_axis_A = ((count_axis_A << 8) & 0xFFFFFF00ll) | (read_byte()  & 0x000000FFll);} while(0)
#define ADD_BYTE_AXIS_B do {count_axis_B = ((count_axis_B << 8) & 0xFFFFFF00ll) | (read_byte()  & 0x000000FFll);} while(0)

#define SELECT_BYTE_0_MSB  do {digitalWrite(HCTL_2032_SEL1, LOW);  digitalWrite(HCTL_2032_SEL2, HIGH);} while (0)
#define SELECT_BYTE_1      do {digitalWrite(HCTL_2032_SEL1, HIGH); digitalWrite(HCTL_2032_SEL2, HIGH);} while (0)
#define SELECT_BYTE_2      do {digitalWrite(HCTL_2032_SEL1, LOW);  digitalWrite(HCTL_2032_SEL2, LOW) ;} while (0)
#define SELECT_BYTE_3_LSB  do {digitalWrite(HCTL_2032_SEL1, HIGH); digitalWrite(HCTL_2032_SEL2, LOW) ;} while (0)

#define SELECT_AXIS_A digitalWrite(HCTL_2032_XY, LOW)
#define SELECT_AXIS_B digitalWrite(HCTL_2032_XY, HIGH)

//
// LCD
//

#define LCD_RE       43
#define LCD_ENABLE   44
#define LCD_D4       39
#define LCD_D5       40
#define LCD_D6       41
#define LCD_D7       42

LiquidCrystal lcd(LCD_RE, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7);           // select the pins used on the LCD panel

// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

int read_LCD_buttons(){               // read the buttons
    adc_key_in = analogRead(0);       // read the value from the sensor 

    // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
    // we add approx 50 to those values and check to see if we are close
    // We make this the 1st option for speed reasons since it will be the most likely result

    if (adc_key_in > 1000) return btnNONE; 

    // For V1.1 us this threshold
    if (adc_key_in < 50)   return btnRIGHT;  
    if (adc_key_in < 250)  return btnUP; 
    if (adc_key_in < 450)  return btnDOWN; 
    if (adc_key_in < 650)  return btnLEFT; 
    if (adc_key_in < 850)  return btnSELECT;  

   // For V1.0 comment the other threshold and use the one below:
   /*
     if (adc_key_in < 50)   return btnRIGHT;  
     if (adc_key_in < 195)  return btnUP; 
     if (adc_key_in < 380)  return btnDOWN; 
     if (adc_key_in < 555)  return btnLEFT; 
     if (adc_key_in < 790)  return btnSELECT;   
   */

    return btnNONE;                // when all others fail, return this.
}

/**
 * EEPROM
 *
 * stores axis labels
 *  0 - 0xAA  (security byte)
 *  1 - 0x55  (security byte)
 *  2 - index
 *  3-5 - ascii codesd to display at beginning of top line
 *  6-8 - ascii codesd to display at beginning of boottom line
 */

#include <EEPROM.h>

void next_AXIS_codes() {
  
  uint8_t index = 0;

 if ((EEPROM.read(0) == 0xAA) && (EEPROM.read(1) == 0x55)) { 
    index = EEPROM.read(2) + 1 ;
    if (index > 2) index = 0;
  }

  EEPROM.write(0, 0xAA);
  EEPROM.write(1, 0x55);
  EEPROM.write(2, index);
  switch(index) {
    case 0: {
      EEPROM.write( 3, 'X');
      EEPROM.write( 4, ' ');
      EEPROM.write( 5, ':');
      EEPROM.write( 6, 'E');
      EEPROM.write( 7, '0');
      EEPROM.write( 8, ':');
      break;
    }                 
    case 1: {         
      EEPROM.write( 3, 'Y');
      EEPROM.write( 4, ' ');
      EEPROM.write( 5, ':');
      EEPROM.write( 6, 'Y');
      EEPROM.write( 7, '2');
      EEPROM.write( 8, ':');
      break;
    }                 
    case 2: {         
      EEPROM.write( 3, 'Z');
      EEPROM.write( 4, ' ');
      EEPROM.write( 5, ':');
      EEPROM.write( 6, 'Z');
      EEPROM.write( 7, '2');
      EEPROM.write( 8, ':');
      break;
    }
  }

  for (uint8_t i = 0; i < 3; i++) {
    lcd.setCursor(i,0);
    lcd.print((char)EEPROM.read(i + 3));
  }
  for (uint8_t i = 0; i < 3; i++) {
    lcd.setCursor(i,1);
    lcd.print((char)EEPROM.read(i + 6));
  }  
}  

void reset_axis_A() {
  digitalWrite(HCTL_2032_RSTX ,LOW );   // axis A counter to zero
  digitalWrite(HCTL_2032_RSTX ,HIGH);   // allow axis A counter to run
  lcd.setCursor(4,0);                   // move cursor to top line and 9 spaces over
  lcd.print("           ");             // clear display
}
  
void reset_axis_B() {  
  digitalWrite(HCTL_2032_RSTY ,LOW );   // axis B counter to zero
  digitalWrite(HCTL_2032_RSTY ,HIGH);   // allow axis B counter to run
  lcd.setCursor(4,1);                   // move cursor to top line and 9 spaces over
  lcd.print("           ");             // clear display

}


int8_t read_byte() {                          // note that the bit order does not match HCTL_2032 data sheet
  int8_t data = 0;
  data |= digitalRead(HCTL_2032_D0)       ;
  data |= digitalRead(HCTL_2032_D1) << 3  ;
  data |= digitalRead(HCTL_2032_D2) << 2  ;
  data |= digitalRead(HCTL_2032_D3) << 1  ;
  data |= digitalRead(HCTL_2032_D4) << 4  ;
  data |= digitalRead(HCTL_2032_D5) << 5  ;
  data |= digitalRead(HCTL_2032_D6) << 6  ;
  data |= digitalRead(HCTL_2032_D7) << 7  ;
  return data;
}

int32_t count_axis_A;
int32_t count_axis_B;
int32_t count_axis_A_prev = 0;
int32_t count_axis_B_prev = 0; 
uint16_t debounce_counter = 0;


void setup(){
  lcd.begin(16, 2);               // start the library
  
if ((EEPROM.read(0) == 0xAA) && (EEPROM.read(1) == 0x55)) {  // get axis codes
    for (uint8_t i = 0; i < 3; i++) {
      lcd.setCursor(i,0);
      lcd.print((char)EEPROM.read(i + 3));
    }
    for (uint8_t i = 0; i < 3; i++) {
      lcd.setCursor(i,1);
      lcd.print((char)EEPROM.read(i + 6));
    }  
  }
  else 
    next_AXIS_codes();                                        // if invalid then get first set
  
  pinMode(HCTL_2032_D0 ,INPUT_PULLUP);
  pinMode(HCTL_2032_D1 ,INPUT_PULLUP);
  pinMode(HCTL_2032_D2 ,INPUT_PULLUP);
  pinMode(HCTL_2032_D3 ,INPUT_PULLUP);
  pinMode(HCTL_2032_D4 ,INPUT_PULLUP);
  pinMode(HCTL_2032_D5 ,INPUT_PULLUP);
  pinMode(HCTL_2032_D6 ,INPUT_PULLUP);
  pinMode(HCTL_2032_D7 ,INPUT_PULLUP);
  pinMode(HCTL_2032_SEL1,OUTPUT);
  pinMode(HCTL_2032_SEL2,OUTPUT);
  pinMode(HCTL_2032_EN1 ,OUTPUT);
  pinMode(HCTL_2032_EN2 ,OUTPUT);
  pinMode(HCTL_2032_OE  ,OUTPUT);
  pinMode(HCTL_2032_CLK ,OUTPUT);
  pinMode(HCTL_2032_RSTX,OUTPUT);
  pinMode(HCTL_2032_RSTY,OUTPUT);
  pinMode(HCTL_2032_XY  ,OUTPUT);
  
  digitalWrite(HCTL_2032_SEL1 ,HIGH);   // don't care
  digitalWrite(HCTL_2032_SEL2 ,HIGH);   // don't care
  digitalWrite(HCTL_2032_EN1  ,HIGH);   // count mode: 1x
  digitalWrite(HCTL_2032_EN2  ,HIGH);   // count mode: 1x
  digitalWrite(HCTL_2032_OE   ,HIGH);   // allow data to be latched
  digitalWrite(HCTL_2032_CLK  ,HIGH);   // don't care
  digitalWrite(HCTL_2032_RSTX ,LOW );   // axis A counter to zero
  digitalWrite(HCTL_2032_RSTX ,HIGH);   // allow axis A counter to run
  digitalWrite(HCTL_2032_RSTY ,LOW );   // axis B counter to zero
  digitalWrite(HCTL_2032_RSTY ,HIGH);   // allow axis B counter to run
  digitalWrite(HCTL_2032_XY ,HIGH);   // don't care
   
}//
 
void loop(){
  
// get counters
 
  ENABLE_OUTPUT;  //freezes latches
  
  SELECT_AXIS_A;
  count_axis_A = 0;
  SELECT_BYTE_0_MSB;
  ADD_BYTE_AXIS_A;
  SELECT_BYTE_1;
  ADD_BYTE_AXIS_A; 
  SELECT_BYTE_2;
  ADD_BYTE_AXIS_A;
  SELECT_BYTE_3_LSB;
  ADD_BYTE_AXIS_A;
  
  SELECT_AXIS_B;
  count_axis_B = 0;
  SELECT_BYTE_0_MSB;
  ADD_BYTE_AXIS_B;
  SELECT_BYTE_1;
  ADD_BYTE_AXIS_B; 
  SELECT_BYTE_2;
  ADD_BYTE_AXIS_B;
  SELECT_BYTE_3_LSB;
  ADD_BYTE_AXIS_B;
  
  DISABLE_OUTPUT;  //allows latches to be updated
 
// display counters
    
  if (count_axis_A != count_axis_A_prev) {
    count_axis_A_prev = count_axis_A;
    lcd.setCursor(4,0);     
    lcd.print("           ");             // clear display if new value
  }  
  
  if (count_axis_B != count_axis_B_prev) {
    count_axis_B_prev = count_axis_B;
    lcd.setCursor(4,1);     
    lcd.print("           ");             // clear display if new value
  } 
    
  lcd.setCursor(4,0);             // move cursor to top line and 4 spaces over
  lcd.print(count_axis_A);        // display current count/position
  
  lcd.setCursor(4,1);             // move cursor to bottom line and 4 spaces over
  lcd.print(count_axis_B);        // display current count/position

// check buttons
  
  lcd_key = read_LCD_buttons();   // read the buttons

  switch (lcd_key){               // depending on which button was pushed, we perform an action

     case btnRIGHT:{              //  push button "RIGHT" 
          break;                  //    NOOP
     } 
     case btnLEFT:{               //  push button "LEFT" and show the word on the screen
          break;                  //    NOOP
     }    
     case btnUP:{                 //  push button "UP" and show the word on the screen
          reset_axis_A();       //   clear Axis A counter  
          break;
     }
     case btnDOWN:{               //  push button "DOWN" 
          reset_axis_B();       //   clear Axis B counter    
          break;
     }
     case btnSELECT:{             //  push button "SELECT"
          if (debounce_counter == 0)
            next_AXIS_codes();
          debounce_counter = 200;  // keep resetting debounce_counter if button still pushed
          break;                  //   scroll through axis codes
     }
     case btnNONE:{               //  decr debounce_counter
       if (debounce_counter) debounce_counter--;
          break;                  
     }
  }
}