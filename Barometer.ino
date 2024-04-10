#include <Wire.h>
#include <stdlib.h>

#include "U8glib.h"
#include "SparkFunBME280.h"

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);  // I2C / TWI

// PWM Pins: 3, 5, 6, 9, 10, 11
const int upLEDPin = 9;
const int upMedLEDPin = 10;
const int upHiLEDPin = 11;

const int downLEDPin = 6;
const int downMedLEDPin = 5;
const int downHiLEDPin = 3;


BME280 mySensorB;
float f_press = 0.0;
long l_press = 0;
char buf[20];
char trendBuf[10];
int p_hist[128];
int p_cnt = 0;

int g_max_brightness = 0;

long s_avg;
int sa_cnt = 0;
const int sa_size = 12; // 5 min interval, 1 hour.

long m_avg;
int ma_cnt = 0;
const int ma_size = 36; // 5 min interval, 3 hour.

long l_avg;
int la_cnt = 0;
const int la_size = 72; // 5 min interval, 6 hour.


//////////////////////////////////////////
//
// GDE_Setup
//
//////////////////////////////////////////
void GDE_Setup(void) {
  Serial.begin(9600);
  Serial.println("BMP_280 - Barometer");

  for (int i = 0; i < 128; i++)
  {
    p_hist[i] = i % 64;
  }
  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255, 255, 255);
  }
}

//////////////////////////////////////////
//
// BMP280_Setup
//
//////////////////////////////////////////
void BMP280_Setup(void) {
  mySensorB.setI2CAddress(0x76); //Connect to a second sensor
  if (mySensorB.beginI2C() == false) Serial.println("Sensor B connect failed");
  mySensorB.setMode(MODE_NORMAL); //MODE_SLEEP, MODE_FORCED, MODE_NORMAL is valid. See 3.3
  mySensorB.setFilter(4); //0 to 4 is valid. Filter coefficient. See 3.4.4
  mySensorB.setStandbyTime(7); //0 to 7 valid. Time between readings. See table 27.
  mySensorB.setTempOverSample(16); //0 to 16 are valid. 0 disables temp sensing. See table 24.
  mySensorB.setPressureOverSample(16); //0 to 16 are valid. 0 disables pressure sensing. See table 23.
}

//////////////////////////////////////////
//
// setup
//
//////////////////////////////////////////
void setup(void) {
  // I2C hwd setup
  GDE_Setup();
  BMP280_Setup();

  // LED PWM pin setup
  pinMode(upLEDPin, OUTPUT);
  pinMode(downLEDPin, OUTPUT);
  pinMode(upMedLEDPin, OUTPUT);
  pinMode(downMedLEDPin, OUTPUT);
  pinMode(upHiLEDPin, OUTPUT);
  pinMode(downHiLEDPin, OUTPUT);
  pinMode(8, OUTPUT);

  g_max_brightness = 250;
  pulse_LEDs(true, true);
  delay(500);
  pulse_LEDs(true, true);
  g_max_brightness = 0;
  
}

//////////////////////////////////////////
//
// genMovingAvg
//
// Assemble string for oLED display
// Fade LEDs
//
//////////////////////////////////////////
void genMovingAvg(void) {
  String s_trend;

//  Serial.print("genMovingAvg: ");

  // A 0.2 kPa change in 1 hour is large.
  // A change of 20 in an hour is large.
  // 200 starts full bright / 20 = 10 (+ 10 to get a basic display)
  g_max_brightness = (abs(l_press - s_avg) * 25) + 10;
//  Serial.print("sB: ");
//  Serial.print(g_max_brightness);
  if (l_press > s_avg ) {
    s_trend = "^ ";
    pulse_LEDs(true, false);
  } else if (l_press < s_avg ) {
    s_trend = "V ";
    pulse_LEDs(false, true);
  } else {
    s_trend = "= ";
    g_max_brightness = 80;
    pulse_LEDs(true, true);
  }
  delay(1000);

  // A 0.2 kPa change in 1 hour is large.
  // A 0.6 kPa change in 3 hours is large.
  // A change of 60 in an hour is large.
  // 200 is full bright / 60 = 3 (+ 40 for base indication)
  g_max_brightness = (abs(l_press - m_avg ) * 20) + 10;
//  Serial.print(" mB: ");
//  Serial.print(g_max_brightness);

  if (l_press > m_avg ) {
    s_trend = s_trend + "^ ";
    pulse_LEDs(true, false);
  } else if (l_press < m_avg ) {
    s_trend = s_trend + "V ";
    pulse_LEDs(false, true);
  } else {
    s_trend = s_trend + "= ";
    g_max_brightness = 80;
    pulse_LEDs(true, true);
  }
  delay(1000);

  // A 0.2 kPa change in 1 hour is large.
  // A 1.2 kPa change in 6 hours is large.
  // A change of 120 in an hour is large.
  // 200 is full bright / 120 = 2
  g_max_brightness = (abs(l_press - l_avg ) * 15) + 10;
//  Serial.print(" lB: ");
//  Serial.print(g_max_brightness);

  if (l_press > l_avg ) {
    s_trend = s_trend + "^ ";
    pulse_LEDs(true, false);
  } else if (l_press < l_avg ) {
    s_trend = s_trend + "V ";
    pulse_LEDs(false, true);
  } else {
    s_trend = s_trend + "= ";
    g_max_brightness = 80;
    pulse_LEDs(true, true);
  }
  delay(1000);

  s_trend.toCharArray(trendBuf, 6);
//  Serial.println(" genMovingAvg - end");
}

//////////////////////////////////////////
//
// draw
//
// Update oLED display
//
//////////////////////////////////////////
void draw(void) {

  u8g.setFont(u8g_font_unifont);
  dtostrf(f_press, 10, 3, buf);
  u8g.drawStr( 0, 15, buf);
  u8g.drawStr( 85, 15, trendBuf);

  for (int i = 0; i < 128; i++)
  {
    u8g.drawPixel(i, (64 - (p_hist[ (i + p_cnt) % 128 ] ) / 2) );
  }
}

//////////////////////////////////////////
//
// pulse_LEDs
//
//////////////////////////////////////////
// Eventual goal is to spend 5 minutes looping through
// pulsing in a breathing ramp up / down 3 sets of LEDs
// one pair for each of the short, medium and long term
// averages.
// Typical loop - repeat 10 times, then measure again:
//  2 seconds fade short
//  1 seconds off
//  2 seconds fade medium
//  1 seconds off
//  2 seconds fade long
//  22 seconds off
void pulse_LEDs(bool b_PulseUp, bool b_PulseDown) {
  int curr_ms;
  int brightness = 1;
  int lowB = 1;
  int medB = 0;
  int hiB = 0;
  int fadeAmount = 5;
  int stepDelay = 45;

  g_max_brightness = (g_max_brightness < 255 ? g_max_brightness : 255);

  //  fadeAmount = (g_max_brightness / 50) + 1;   // 50 steps. 250 / 50 = 5
  //  Serial.print("Max bright: ");
  //  Serial.print(g_max_brightness);
  //  Serial.print(" Step size: ");
  //  Serial.println(fadeAmount);

  // Prime glow:
  analogWrite(upLEDPin, (b_PulseUp ? lowB : 0));
  analogWrite(downLEDPin, (b_PulseDown ? lowB : 0));
  delay(stepDelay * 2);

  // Fade loop
  // 3 phases
  //
  while (brightness > 0) {
    analogWrite(upLEDPin, (b_PulseUp ? lowB : 0));
    analogWrite(downLEDPin, (b_PulseDown ? lowB : 0));
    analogWrite(upMedLEDPin, (b_PulseUp ? medB : 0));
    analogWrite(downMedLEDPin, (b_PulseDown ? medB : 0));
    analogWrite(upHiLEDPin, (b_PulseUp ? hiB : 0));
    analogWrite(downHiLEDPin, (b_PulseDown ? hiB : 0));

    brightness += fadeAmount;

    if (brightness <= 85) {
      lowB = brightness * 3;
      medB = 0;
      hiB = 0;
    } else if (brightness >= 85 && brightness <= 170) {
      medB = (brightness - 85) * 3;
      hiB = 0;
    } else
      hiB = (brightness - 170) * 3;

    if (brightness >= g_max_brightness ) {
      fadeAmount = - fadeAmount;
    }

    delay(stepDelay);
  }

  // Ensure LEDs are off at end of fade.
  analogWrite(upLEDPin, 0);
  analogWrite(downLEDPin, 0);
}

//////////////////////////////////////////
//
// buildAvg
//
//////////////////////////////////////////
void buildAvg() {
  int i, idx;

  Serial.print("p_hist dump");
  for (int i = 0; i < 128; i++)
  {
    if ( (i % 16) == 0) Serial.println();
    Serial.print( p_hist[ (i + p_cnt) % 128 ] );
    Serial.print(" ");
  }
  Serial.println();

//  Serial.print("buildAvg. l_press: ");
//  Serial.print(l_press);

  // Extract meaningful range.
  // Raw data is 95,000 +/- 5000
  // -90000 and divide by 100
  // Reduced range should be 100 - 1500
  p_cnt++;
  if  (p_cnt > 127) p_cnt = 0;
  p_hist[p_cnt] = l_press;

  sa_cnt++;
  if (sa_cnt > sa_size) sa_cnt = sa_size;
  ma_cnt++;
  if (ma_cnt > ma_size) ma_cnt = ma_size;
  la_cnt++;
  if (la_cnt > la_size) la_cnt = la_size;

  // Short average (~1 hour)
  //  s_avg = ((s_avg * sa_cnt) + data) / (sa_cnt + 1);
  s_avg = 0;
  for (i = 0; i < sa_cnt; i++) {
    idx = p_cnt - i;
    if (idx < 0) idx += 127;
    s_avg = s_avg + p_hist[ idx ];
  }
  s_avg = s_avg / sa_cnt;
//  Serial.print(" s ");
//  Serial.print(s_avg);
//  Serial.print("/");
//  Serial.print(sa_cnt);

  //  m_avg = ((m_avg * ma_cnt) + data) / (ma_cnt + 1);
  m_avg = 0;
  for (i = (ma_cnt - sa_cnt); i < ma_cnt; i++) {
    idx = p_cnt - i;
    if (idx < 0) idx += 127;
    m_avg = m_avg + p_hist[ idx ];
  }
  m_avg = m_avg / sa_cnt; // sa_cnt is both the short average, and the # of samples in the mid/long ave
//  Serial.print(" m ");
//  Serial.print(m_avg);
//  Serial.print("/");
//  Serial.print(ma_cnt);

  //  l_avg = ((l_avg * la_cnt) + data) / (la_cnt + 1);
  l_avg = 0;
  for (i = (la_cnt - sa_cnt); i < la_cnt; i++) {
    idx = p_cnt - i;
    if (idx < 0) idx += 127;
    l_avg = l_avg + p_hist[ idx ];
  }
  l_avg = l_avg / sa_cnt; // sa_cnt is both the short average, and the # of samples in the mid/long ave
//  Serial.print(" l ");
//  Serial.print(l_avg);
//  Serial.print("/");
//  Serial.print(la_cnt);

//  Serial.println(" end-buildAvg");
}

//////////////////////////////////////////
//
// loop
//
//////////////////////////////////////////
void loop(void) {
  int waitCnt;

  f_press = mySensorB.readFloatPressure();
  l_press = (((long)f_press) - 90000) / 100;
  buildAvg();

  // Display loop takes ~10 seconds
  // waitCnt is how many times to run the display loop
  // before acquiring new data
  // We have 128 bits to track 12 hour average. The requires
  // 5.625 minutes / bit.
  // 5.625 minutes = 337.5 seconds
  // 337.5 seconds / 10 seconds = 34 loops
  waitCnt = 34;
  //  waitCnt = 6; // one minute between bits for debugging
  while (waitCnt > 0) {
    genMovingAvg(); // get trend, pulse LEDs, update display string
    u8g.firstPage();
    do {
      draw();
    } while ( u8g.nextPage() );

    delay(5000);
    waitCnt -= 1;
  }

  //  delay(337500); // 12 hours across 128 bits (5.625 minutes / bit)
}
