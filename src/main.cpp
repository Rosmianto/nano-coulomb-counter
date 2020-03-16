#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>


#define LTC_INTERRUPT  2
#define LTC_POLARITY   4
#define LTC_SHUTDOWN   5

volatile double load_mAh = 0.0;
volatile boolean isrflag;
volatile long int time, lasttime;
volatile double mA;
volatile unsigned long elapsedTime = 0;
double ah_quanta = 0.17067759; // mAh for each INT

Adafruit_SSD1306 display(128, 64, &Wire, -1);
Adafruit_INA219 ina219;

void myISR();

void setup() {
  ina219.begin();
  ina219.setCalibration_16V_400mA();
  // ina219.setCalibration_32V_1A();

  pinMode(LTC_INTERRUPT, INPUT);
  pinMode(LTC_POLARITY, INPUT);
  pinMode(LTC_SHUTDOWN, OUTPUT);
  
  digitalWrite(LTC_SHUTDOWN, LOW);
  Serial.begin(115200);
  // Serial.println("LTC4150 Coulomb Counter");
  Serial.println("Current");
  isrflag = false;
  attachInterrupt(digitalPinToInterrupt(LTC_INTERRUPT), myISR, FALLING);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(500);
  display.setRotation(2);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  // Display static text
  display.println("Load: 0 mAh");
  display.println("Time: 0 s");
  display.println("Current: 0 mA");
  display.display(); 
  delay(100);

  digitalWrite(LTC_SHUTDOWN, HIGH);
}

void loop() {

  if (isrflag) {    
    isrflag = false;
  }

  float current_mA = ina219.getCurrent_mA();
  float busvoltage = ina219.getBusVoltage_V();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("INA219 I: ");
  if (current_mA < 99.0) display.print('0');
  if (current_mA < 9.0)  display.print('0');
  display.println(String(current_mA) + " mA");
  display.print("INA219 V: ");
  display.println(String(busvoltage) + " V");

  display.print("LT4150: "); display.print(mA); display.println(" mA");
  display.print("Load: "); display.print(load_mAh); display.println(" mAh");
  display.println("Time: " + String(elapsedTime) + " s");
  display.display();
  Serial.println(current_mA);
  delay(10);
}

void myISR() // Run automatically for falling edge on D3 (INT1)
{
  static boolean polarity;
  
  // Determine delay since last interrupt (for mA calculation)
  // Note that first interrupt will be incorrect (no previous time!)
  lasttime = time;
  time = micros();

  // Get polarity value 
  polarity = digitalRead(LTC_POLARITY);
  if (polarity) // high = charging
  {
    load_mAh -= ah_quanta;
  }
  else // low = discharging
  {
    load_mAh += ah_quanta;
  }

  // // Calculate mA from time delay (optional)
  mA = 614.4/((time-lasttime)/1000000.0);

  // // If charging, we'll set mA negative (optional)
  if (polarity) mA = mA * -1.0;
  
  // Set isrflag so main loop knows an interrupt occurred
  isrflag = true;

  elapsedTime = millis() / 1000.0;
}