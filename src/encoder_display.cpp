/*************************************************************************************

  Mark Bramwell, July 2010

  This program will test the LCD panel and the buttons.When you push the button on the shield，
  the screen will show the corresponding one.
 
  Connection: Plug the LCD Keypad to the UNO(or other controllers)

**************************************************************************************/

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
#define HCTL_2032_SEL1    10
#define HCTL_2032_SEL2    11
#define HCTL_2032_EN1     12
#define HCTL_2032_EN2     13
#define HCTL_2032_OE      14
#define HCTL_2032_CLK     15
#define HCTL_2032_RSTX    16
#define HCTL_2032_RSTY    17
#define HCTL_2032_XY      18
#define HCTL_2032_DATA_PORT  PORTD

#define ENABLE_OUTPUT  digitalWrite(HCTL_2032_OE, LOW)
#define DISABLE_OUTPUT digitalWrite(HCTL_2032_OE, HIGH)

#define ADD_BYTE_AXIS_A do {digitalWrite(HCTL_2032_CLK, LOW); count_axis_A = (count_axis_A << 8) | HCTL_2032_DATA_PORT; digitalWrite(HCTL_2032_CLK, HIGH);} while(0)
#define ADD_BYTE_AXIS_B do {digitalWrite(HCTL_2032_CLK, LOW); count_axis_B = (count_axis_B << 8) | HCTL_2032_DATA_PORT; digitalWrite(HCTL_2032_CLK, HIGH);} while(0)

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

int32_t count_axis_A;
int32_t count_axis_B; 
int8_t temp_byte;

void setup(){
  lcd.begin(16, 2);               // start the library
  lcd.setCursor(0,0);             // set the LCD cursor   position 
  lcd.print("Axis L:");    
  lcd.setCursor(0,1);             // set the LCD cursor   position 
  lcd.print("Axis R:");    
   
  pinMode(HCTL_2032_D0 ,INPUT);
  pinMode(HCTL_2032_D1 ,INPUT);
  pinMode(HCTL_2032_D2 ,INPUT);
  pinMode(HCTL_2032_D3 ,INPUT);
  pinMode(HCTL_2032_D4 ,INPUT);
  pinMode(HCTL_2032_D5 ,INPUT);
  pinMode(HCTL_2032_D6 ,INPUT);
  pinMode(HCTL_2032_D7 ,INPUT);
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
   
}
 
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
  
  lcd.setCursor(9,0);             // move cursor to top line and 9 spaces over
  lcd.print(count_axis_A);        // display current count/position
  
  lcd.setCursor(9,1);             // move cursor to bottom line and 9 spaces over
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
          count_axis_A = 0;       //   clear Axis A counter   
          break;
     }
     case btnDOWN:{               //  push button "DOWN" 
          count_axis_B = 0;       //   clear Axis B counter     
          break;
     }
     case btnSELECT:{             //  push button "SELECT"
          break;                  //    NOOP
     }
     case btnNONE:{               //  No action
          break;                  //    NOOP
     }
  }
}