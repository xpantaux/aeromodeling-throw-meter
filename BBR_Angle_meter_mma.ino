/*
   --------------------------------------------------
   Uses nearly the same schematic designed around the
   ADXL345. On AXL345 CS input is tight to +3V on
   the MMA8451 CS0 is connected to +3V.
   On this version the push button is monitored using
   an interrupt
   Have fun!
   --------------------------------------------------
*/

#include <Wire.h>
#include <Adafruit_MMA8451.h>         // MMA8451 library            from https://github.com/adafruit/Adafruit_MMA8451_Library
#include <Adafruit_Sensor.h>          // comes with the above
#include <U8g2lib.h>                  // Oled U8g2 library          from https://github.com/olikraus/U8g2_Arduino/archive/master.zip

int action = 0;
double corde,  ref_angle;
const float pi = M_PI;
const int buttonPin = 2;
volatile int bp_pushed = 0;
volatile  unsigned long bp_down = 0 ;
volatile  unsigned long bp_up = 0;
volatile  unsigned long bp_ = 0;

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA);   // pin remapping with ESP8266 HW I2C

//----------------------------------------------------------------------------------------------------------------------------------
Adafruit_MMA8451 mma = Adafruit_MMA8451();
//----------------------------------------------------------------------------------------------------------------------------------
double read_angle() {                             // Function returning the current rotation value along X axis - in degrees
  //----------------------------------------------------------------------------------------------------------------------------------
  int x, y, z;
  double l_angle = 0;
  double ll_angle = 0;
  int mm = 50;
  for (int nn = 1;  nn <= mm; nn++) {               // Average value computed over roughly 100ms
    mma.read();                                     // Read the accelerometer values and store them in variables declared above x,y,z
    l_angle = atan2(mma.y, mma.z) * 57.3;           // Compute rotation angle along X axis of accelerometer
    ll_angle = ll_angle + (l_angle);
  }
  l_angle = round(ll_angle / mm * 10);
  ll_angle = l_angle / 10;
  return ll_angle;
}
//----------------------------------------------------------------------------------------------------------------------------------
void init_angle() {
  //----------------------------------------------------------------------------------------------------------------------------------
  double ra = 0;
  delay(200);
  ra = read_angle();                              // Initialize the actual angle as the reference angle
  ref_angle = ra;
}
//----------------------------------------------------------------------------------------------------------------------------------
void setup() {
  //----------------------------------------------------------------------------------------------------------------------------------
  delay(500);
  corde = 50;                                     // Chord width value
  Serial.begin(9600);
  //  Serial.println("Init done");
  u8g2.begin();                                   // Start Oled
  affiche_init();
  delay(500);
  if (! mma.begin()) {                            // Try to start MMA, if fails then display error message
    u8g2.firstPage();
    do {
      u8g2.setFontDirection(0);
      u8g2.setFont(u8g2_font_t0_14_tf);
      u8g2.setCursor(18, 15);
      u8g2.print("Erreur MMA8451");
    } while ( u8g2.nextPage() );
    while (1);
  }
  mma.setRange(MMA8451_RANGE_2_G);
  pinMode(buttonPin, INPUT);                      // define the Push Button input
  attachInterrupt(digitalPinToInterrupt(buttonPin), ISRbutton, CHANGE); // Interrupt on CHANGE button 2
  init_angle();
}
//----------------------------------------------------------------------------------------------------------------------------------
void ISRbutton()  {
  //----------------------------------------------------------------------------------------------------------------------------------
  bp_ = millis();
  if (bp_ - bp_up > 200) {
    if (digitalRead(buttonPin) == LOW) {
      bp_down = millis();
    }
    if (digitalRead(buttonPin) == HIGH) {
      bp_up = millis();
      if (bp_up - bp_down > 1000) {
        bp_pushed = 1;
      } else  bp_pushed = 2;
    }
  }
}
//----------------------------------------------------------------------------------------------------------------------------------
String cnv_flt2str(float num, int car, int digit) { // Convert a float variable into a string with a specific number of digits
  //----------------------------------------------------------------------------------------------------------------------------------
  float tmp_num, expo;
  String str = "";
  switch (digit) {
    case 0: expo = 1; break;
    case 1: expo = 10; break;
    case 2: expo = 100; break;
    case 3: expo = 1000; break;
    case 4: expo = 10000; break;
    default: expo = 0; break;
  }
  if (expo != 0) {
    str = String(int(num));
    if (expo > 1)  {
      tmp_num = abs(num) - int(abs(num));
      str = str + "." + String(int(tmp_num * expo));
    }
  }
  while (str.length() < car) {
    str = " " + str;
  }
  return str;
}
//----------------------------------------------------------------------------------------------------------------------------------
void aff_menu() {
  //----------------------------------------------------------------------------------------------------------------------------------
  int action = 0,  l_pas = 1;
  double l_ang = 0, l_act = 0, l_posi = 0, l_ref = 0;
  u8g2.firstPage();                                                 // Display values
  do {
    u8g2.setFontDirection(0);
    u8g2.setFont(u8g2_font_t0_14_tf);
    u8g2.setCursor(18, 15);
    u8g2.print("Corde : " + cnv_flt2str(corde, 4, 1) + "mm");
    u8g2.drawBox(64 - 7, 22, 14, 10);
    u8g2.drawFrame(64 - 20, 22, 40, 10);
    u8g2.drawFrame(64 - 45, 22, 90, 10);
    u8g2.drawFrame(0, 22, 128, 10);
    u8g2.setFont(u8g2_font_micro_tr);
    u8g2.setCursor(64 - 1, 30);
    u8g2.print("0");
    u8g2.setCursor(64 - 19, 30);
    u8g2.print("-.1");
    u8g2.setCursor(64 - 45 + 12, 30);
    u8g2.print("-1");
    u8g2.setCursor(64 - 63 + 5, 30);
    u8g2.print("-10");
    u8g2.setCursor(64 + 19 - 9, 30);
    u8g2.print(".1");
    u8g2.setCursor(64 + 45 - 14, 30);
    u8g2.print("1");
    u8g2.setCursor(64 + 63 - 14, 30);
    u8g2.print("10");
  } while ( u8g2.nextPage() );
  delay(1500);
  l_ref = read_angle();
  do {
    l_ang = read_angle() - l_ref;                                       // read current angle
    if (abs(l_ang) >= 7) {                                              // If angle over 7 degrees
      if (abs(l_ang) < 20) {
        l_act = 0.1;
      }
      else {
        if (abs(l_ang) < 45) l_act = 1;
        else {
          l_act = 10;
        }
      }
      if (l_ang > 0) l_pas = 1; else l_pas = -1;
    }
    else l_act = 0;
    //  Serial.println("angle :" + String(abs(ang)) + " act " + String(act));
    if (l_act != 0) {
      corde = corde + (l_act * l_pas);
    }

    l_posi = l_ang;

    if (l_posi >= 64) {
      l_posi = 64;
    }
    else {
      if (l_posi < -64) {
        l_posi = -64;
      }
    }
    l_pas = 64 + l_posi;
    u8g2.firstPage();                                                 // Display values
    do {
      u8g2.setFontDirection(0);
      u8g2.setFont(u8g2_font_t0_14_tf);
      u8g2.setCursor(18, 15);
      u8g2.print("Corde : " + cnv_flt2str(corde, 4, 1) + "mm");
      u8g2.drawBox(64 - 7, 22, 14, 10);
      u8g2.drawFrame(64 - 20, 22, 40, 10);
      u8g2.drawFrame(64 - 45, 22, 90, 10);
      u8g2.drawFrame(0, 22, 128, 10);
      u8g2.setFont(u8g2_font_micro_tr);
      u8g2.setCursor(64 - 1, 30);
      u8g2.print("0");
      u8g2.setCursor(64 - 19, 30);
      u8g2.print("-.1");
      u8g2.setCursor(64 - 45 + 12, 30);
      u8g2.print("-1");
      u8g2.setCursor(64 - 63 + 5, 30);
      u8g2.print("-10");
      u8g2.setCursor(64 + 19 - 9, 30);
      u8g2.print(".1");
      u8g2.setCursor(64 + 45 - 14, 30);
      u8g2.print("1");
      u8g2.setCursor(64 + 63 - 14, 30);
      u8g2.print("10");
      u8g2.drawLine(l_pas, 18, l_pas, 34);
      u8g2.drawLine(l_pas - 1, 18, l_pas - 1, 20);
      u8g2.drawLine(l_pas + 1, 18, l_pas + 1, 20);
    } while ( u8g2.nextPage() );
    delay(100);
  } while (bp_pushed == 0);
  bp_pushed = 0;
}
//----------------------------------------------------------------------------------------------------------------------------------
void affiche_init() {
  //----------------------------------------------------------------------------------------------------------------------------------
  u8g2.firstPage();                                                 // Display values
  do {
    u8g2.setFontDirection(0);
    u8g2.setFont(u8g2_font_t0_14_tf);
    u8g2.setCursor(18, 24);
    u8g2.print("INIT EN COURS");
  } while ( u8g2.nextPage() );
}
//----------------------------------------------------------------------------------------------------------------------------------
void affiche(String l_angle, String l_corde, String l_debat) {
  //----------------------------------------------------------------------------------------------------------------------------------
  u8g2.firstPage();                                                 // Display values
  do {
    u8g2.setFontDirection(0);
    u8g2.setFont(u8g2_font_t0_11_tf);
    u8g2.setCursor(1, 10);
    u8g2.print("Angle deg -->");
    u8g2.setFont(u8g2_font_crox4tb_tn);
    u8g2.setCursor(78, 15);
    u8g2.print(l_angle);
    u8g2.setFont(u8g2_font_t0_11_tf);
    u8g2.setCursor(1, 21);
    u8g2.print("Corde " + l_corde + "mm");
    u8g2.setCursor(1, 31);
    u8g2.print("Debat mm --->");
    u8g2.setFont(u8g2_font_crox4tb_tn);
    u8g2.setCursor(78, 32);
    u8g2.print(l_debat);
  } while ( u8g2.nextPage() );
}
//----------------------------------------------------------------------------------------------------------------------------------
void loop() {                                     // Main loop
  //----------------------------------------------------------------------------------------------------------------------------------
  float x_rot = 0, aff_angle = 0, angle = 0, debat = 0;
  int act = 0;

  if (bp_pushed != 0) {
    do {
      delay(10);
    } while (digitalRead(buttonPin) == LOW);
    action = bp_pushed;
    bp_pushed = 0;
  }

  switch (action) {
    case (1):
      aff_menu();
      action = 2;
      break;
    case (2):
      affiche_init();
      init_angle();
      action = 0;
      break;
    default:
      break;
  }
  x_rot = read_angle();                                               // read current angle
  x_rot = ref_angle - x_rot;                                          // compute angle variation vs. reference angle
  angle = (x_rot / 180) * pi;                                         // angle value converted into radian
  //  Serial.println("angle :" + String(angle));
  debat = sqrt(2 * sq(corde) - (2 * sq(corde) * cos(angle)));         // throw computation in same units as chord
  affiche(cnv_flt2str(x_rot, 6, 1), cnv_flt2str(corde, 4, 1), cnv_flt2str(debat, 6, 1));
}
