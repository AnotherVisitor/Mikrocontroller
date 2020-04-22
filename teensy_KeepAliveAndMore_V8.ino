/*
 * name: 
 * teensy_KeepAliveAndMore_V8
 *  
 * hardware: Teensy 3.2 
 * (with a solded crystal to run the clock)
 *
 * A I2C display LCD1602 is connected to SDA1(D18) and SCL1(D19)
 * (pull-ups are active - 2 x 4k7)
 *
 * 5 pushbuttons are connected to GND and ports D5, D5, D7, D8 and D9
 * A rotary switch is connected to D10 and D11. Its button to D12.
 * Their pullup resistors are activated.
 *
 * settings:
 * To use mouse() and keyboard commands,
 * select Mouse, Keyboard and Joystick from the "Tools > USB Type" menu
 * extra:
 * 
 * additional info:
 * https://github.com/luni64/TeensyTimerTool/tree/master/src
 * for new released timer tool
 *
 * comments:
 * Wday - Sunday = 1
 *
 * whats new:
 * switched back to the PJRC Encoder library.
 * works if you do not enable the pull-ups on the rotary!
 * 
 * when:
 * 23. April 2020
 * 
 * who:
 * Heiko Wagner
 */

#define ENCODER_USE_INTERRUPTS
//#define ENCODER_OPTIMIZE_INTERRUPTS
//#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>

//I2C 16x2 connected to SDA1/SCL1
//LiquidCrystal_I2C lcd(0x3f,16,4);
LiquidCrystal_I2C lcd(0x27,16,4);

//IntervalTimer object 
IntervalTimer everySecond;

//Teensy 3.2 - Left side
//                    // PIN01 // GND 
#define PIN_D0     0  // PIN02 // RX1
#define PIN_D1     1  // PIN03 // TX1
#define PIN_D2     2  // PIN04 // 
#define PIN_D3     3  // PIN05 // CAN-TX
#define PIN_D4     4  // PIN06 // CAN-RX
#define PIN_D5     5  // PIN07 // 
#define PIN_D6     6  // PIN08 // 
#define PIN_D7     7  // PIN09 // RX3
#define PIN_D8     8  // PIN10 // TX3
#define PIN_D9     9  // PIN11 // RX2
#define PIN_D10   10  // PIN12 // TX2 SPI-CS
#define PIN_D11   11  // PIN13 //     SPI-DOUT
#define PIN_D12   12  // PIN14 //     SPI-DIN
//Teensy 3.2 - Right side
#define LED       13  // PIN15 // LED SPI-SCK 
#define PIN_D14   14  // PIN16 // A0
#define PIN_D15   15  // PIN17 // A1
#define PIN_D16   16  // PIN18 // A2
#define PIN_D17   17  // PIN19 // A3
#define PIN_D18   18  // PIN20 // A4 I2C-SDA0
#define PIN_D19   19  // PIN21 // A5 I2C-SCL0
#define PIN_D20   20  // PIN22 // A6
#define PIN_D21   21  // PIN23 // A7
#define PIN_D22   22  // PIN24 // A8
#define PIN_D23   23  // PIN25 // A9
//                    // PIN26 // 3V3 250mA max
//                    // PIN27 // AGND
//                    // PIN28 // Vin

//ROTxxx must be even!
#define ROTMAX     5  // upper rotary limit
#define ROTMIN     0  // lower rotary limit
#define BACKLIGHT 30 // Time to keep the backlight on

#define ZEILE1 0
#define ZEILE2 1
#define ZEILE3 2
#define ZEILE4 3

/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  "T"   // Header tag for serial time sync message

//encoder
Encoder enc_Rotary1(PIN_D10, PIN_D11);

///////////////////////////////////////////////////////////////////
//GLOBAL_VARS//GLOBAL_VARS//GLOBAL_VARS//GLOBAL_VARS//GLOBAL_VARS//

int i_out, i_in, i_result;
int l_newPosRotary1;            // actual position of encoder
int l_posRotary1 = 1;           // new position (to check if there was a change)
int i_moveMouse = 0;            // if 1 (the times reached 0) the mouse must be moved
int i_backlight = BACKLIGHT;    // time to keep backlight on (in seconds)

long l_nextMove = 0;            //epoch, the next mouse move happens

char num3[4];                   // string to store integers
char num4[5];                   // string to store integers
char num5[6];                   // string to store integers
char num16[17];                 // string to store integers

const char* const sWeekDayLong[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char* const sWeekDay[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

const char* const sMonthLong[] = {"January", "February", "March", "April", "May", "June", "July", 
                                  "August", "September", "October", "November", "December"};
const char* const sMonth[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};


time_t prevDisplay = 0; // when the digital clock was displayed
time_t t_now;
tmElements_t tm;

 bool bo_displayMode = 0; // 0:Display, 1:Edit
 int i_menue = 0;

//////////////////////////////////////////////////////////////////////
//PROTO//PROTO//PROTO//PROTO//PROTO//PROTO//PROTO//PROTO//PROTO//PROTO

void mouseRightAndLeft(void);
void runEverySecond(void);
void emailGoodyear(void);
void prepareDisplay(void);
void emailGmail(void);
void emailPosteo(void);
void anotherText1(void);
void anotherText2(void);
void updateRot(void);
unsigned long processSyncMessage(void);
time_t  getTeensy3Time(void);

//////////////////////////////////////////////////////////////////////
//SETUP//SETUP//SETUP//SETUP//SETUP//SETUP//SETUP//SETUP//SETUP//SETUP

void setup() { 

   Serial.begin(9600);
  
   pinMode(LED, OUTPUT);
   digitalWrite(LED, HIGH);

   // set the Time library to use Teensy 3.0's RTC to keep time
   setSyncProvider(getTeensy3Time);

   //while (!Serial);  // Wait for Arduino Serial Monitor to open
      //delay(100);
      
   if (timeStatus()!= timeSet) {
      Serial.println("Unable to sync with the RTC");
   } else {
      Serial.println("RTC has set the system time");
   }
   
   //Interrupt timer
   everySecond.begin(runEverySecond, 1000000);  //runs every second

   lcd.init();
   lcd.backlight();
   
//   lcd.setCursor(0,ZEILE1);
//   lcd.print("Hello");
//   lcd.setCursor(0,ZEILE2);
//   lcd.print("2te Zeile");
//   lcd.setCursor(0,ZEILE3);
//   lcd.print("3te Zeile");
//   lcd.setCursor(0,ZEILE4);
//   lcd.print("4te Zeile");
   
   pinMode(PIN_D4, INPUT_PULLUP);  // comment
   pinMode(PIN_D5, INPUT_PULLUP);  // Pushbutton - upper row
   pinMode(PIN_D6, INPUT_PULLUP);  // Pushbutton - bottom row 1
   pinMode(PIN_D7, INPUT_PULLUP);  // Pushbutton - bottom row 2
   pinMode(PIN_D8, INPUT_PULLUP);  // Pushbutton - bottom row 3
   pinMode(PIN_D9, INPUT_PULLUP);  // Pushbutton - bottom row 4
   //pinMode(PIN_D10, INPUT_PULLUP); // Rotary - Rotate1
   //pinMode(PIN_D11, INPUT_PULLUP); // Rotary - Rotate2
   pinMode(PIN_D12, INPUT_PULLUP); // Rotary Pushbutton

   l_nextMove = now()+120;
   //Serial.println(now());
   //Serial.println(l_nextMove);

   delay(100);

   lcd.setCursor(0,ZEILE2);
   lcd.print("Online");
   
   Serial.println("EOF setup()");
} // end of setup

////////////////////////////////////////////////////////////////////
//LOOP//LOOP//LOOP//LOOP//LOOP//LOOP//LOOP//LOOP//LOOP//LOOP//LOOP//

void loop() {

   if (!digitalRead(PIN_D5)){emailGoodyear(); delay(250); i_backlight = BACKLIGHT; lcd.backlight();}  
   if (!digitalRead(PIN_D6)){emailGmail();    delay(250); i_backlight = BACKLIGHT; lcd.backlight();}
   if (!digitalRead(PIN_D7)){emailPosteo();   delay(250); i_backlight = BACKLIGHT; lcd.backlight();}
   if (!digitalRead(PIN_D8)){anotherText1();  delay(250); i_backlight = BACKLIGHT; lcd.backlight();}
   if (!digitalRead(PIN_D9)){anotherText2();  delay(250); i_backlight = BACKLIGHT; lcd.backlight();}
   if (!digitalRead(PIN_D12)){lcd.init();                 i_backlight = BACKLIGHT;}

//   Set time via serial
//   if (Serial.available()) {
//      Serial.println("(time)");
//      t_now = processSyncMessage();
//      if (t_now != 0) {
//         Teensy3Clock.set(t_now); // set the RTC
//         setTime(t_now);
//      } // end of if
//   }  // end of if
    
   l_newPosRotary1 = enc_Rotary1.read()/2;
    
   if (l_posRotary1 != l_newPosRotary1)
   {
      //Serial.println(l_newPosRotary1);
    
      if (l_newPosRotary1 > ROTMAX)
      {
         Serial.println(F("At upper limit."));
         enc_Rotary1.write(ROTMIN*2);
         l_newPosRotary1 = ROTMIN;
      } // end of if
      
      if (l_newPosRotary1 < ROTMIN)
      {
         Serial.println(F("At lower limit."));
         enc_Rotary1.write(ROTMAX*2);
         l_newPosRotary1 = ROTMAX;
      } // end of else if

         i_menue = l_newPosRotary1;
         l_posRotary1 = l_newPosRotary1;
         
         i_backlight = BACKLIGHT; 
         lcd.backlight();
      
         switch (i_menue)
         {
            case 0: // Menue #0
               Serial.println("Menue #0");
               //memset(num16, 0, sizeof(num16));
               sprintf(num16, "%1d: %s %4d     ", i_menue, sMonth[tm.Month], tm.Year+1970);
               lcd.setCursor(0,ZEILE2);
               lcd.print(num16); 
               break; 
            
            case 1: // Menue #1
               Serial.println("Menue #1");
               sprintf(num16, "Menue 1         "); 
               lcd.setCursor(0,ZEILE2);
               lcd.print(num16);
               break; 
   
            case 2: // Menue #2
               Serial.println("Menue #2"); 
               sprintf(num16, "Menue 2         ");
               lcd.setCursor(0,ZEILE2);
               
               lcd.print(num16);
               break; 
       
            case 3: // Menue #3
               Serial.println("Menue #3"); 
               lcd.setCursor(0,ZEILE2);
               sprintf(num16, "Menue 3         "); 
               lcd.print(num16);
               break; 
      
            case 4: // Menue #4
               Serial.println("Menue #4"); 
               lcd.setCursor(0,ZEILE2);
               sprintf(num16, "Menue 4         "); 
               lcd.print(num16);
               break; 
      
            case 5: // Menue #5
               Serial.println("Menue #5"); 
               lcd.setCursor(0,ZEILE2);
               sprintf(num16, "Menue 5         "); 
               lcd.print(num16);
               break; 

            default:
               Serial.println("Default");
               lcd.setCursor(0,ZEILE2);
               sprintf(num16, "Default         "); 
               lcd.print(num16);
               break; 
            
         } // end of switch 
      
   } // end of if (l_posRotary1 !=


   if (i_moveMouse == 1){
      mouseRightAndLeft();
      i_moveMouse = 0;
      Serial.println("MOUSE!");
   } // end of if

   
   
} // end of loop


///////////////////////////////////////////////////////////////
// begin of subs

void mouseRightAndLeft() {
   for (i_out=1; i_out<10; i_out++) {
      //Mouse.move(1, 0);
      delay(25);
   } // end of for
   for (i_out=1; i_out<10; i_out++) {
      //Mouse.move(-1, 0);
     delay(25);
   } // end of for
} // end of mouseRightAndLeft

///////////////////////////////////////////////////////////////
void runEverySecond(){

   if ((l_nextMove-now()) == 0){
      i_moveMouse = 1;
      l_nextMove = now() + 120;
   } // end of if

   i_backlight--;
   if (i_backlight < 0){
      lcd.noBacklight();
      i_backlight = 1;   
   } // end of if

   // flash led
   digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
   t_now = Teensy3Clock.get();
   breakTime(t_now, tm);

   lcd.setCursor(0,ZEILE1);
   //sprintf(num5, "%02d:%02d", tm.Hour, tm.Minute);
   //sprintf(num16, "%02d:%02d %02d.%02d.%04d", tm.Hour, tm.Minute, tm.Day, tm.Month, tm.Year+1970);
   sprintf(num16, "%02d:%02d:%02d %s %02d.", tm.Hour, tm.Minute, tm.Second, sWeekDay[tm.Wday-1], tm.Day);
   lcd.print(num16);

} // end of runEverySecond
///////////////////////////////////////////////////////////////

void emailGoodyear(){
   //Keyboard.print("heiko_wagner@goodyear.com");
} // end of emailGoodyear
///////////////////////////////////////////////////////////////

void emailGmail(){
   //Keyboard.print("heiko.wagner@gmail.com");
} // end of emailGmail

///////////////////////////////////////////////////////////////

void emailPosteo(){
   //Keyboard.print("heiko.wagner@posteo.de");
} // end of emailPosteo
///////////////////////////////////////////////////////////////

void anotherText1(){
   //Keyboard.print("AnotherText1");
} // end of anotherText1
///////////////////////////////////////////////////////////////

void anotherText2(){
   //Keyboard.print("AnotherText2");
} // end of anotherText2
///////////////////////////////////////////////////////////////

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
} // end of getTeensyTime
///////////////////////////////////////////////////////////////
// run date +%s on linux to get epoch.
//Then send it by adding a "T" to it (e.g.: T1587325797)
 
unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     return pctime;
     if( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
       pctime = 0L; // return 0 to indicate that the time is not valid
     }
  }
  return pctime;
}  // end of processSyncMessage

///////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//END/OF/THE/WORLD//END/OF/THE/WORLD//END/OF/THE/WORLD//END/OF/THE/WORLD
////////////////////////////////////////////////////////////////////////
