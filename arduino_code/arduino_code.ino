#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <dht.h>
#include <EEPROM.h>
#include "GravityTDS.h"
#include "DFRobot_PH.h"
#include "DFRobot_EC.h"
#include <SoftwareSerial.h>

int displayIndex = 0;
int buttonState = 0;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 100;
int i = 0;
int j = 0; 
unsigned long startTime = 0;

float lastMoistVal = 0;
float lastTdsVal = 0;
float lastPhVal = 0;
float lastEcVal = 0;
float lastHumVal = 0;
float lastTempVal = 0;
float lastDistVal = 0;
float lastLuxVal = 0;

SoftwareSerial espSerial(10, 11);
String str;
LiquidCrystal_I2C lcd(0x27, 16, 2);
dht DHT;
GravityTDS gravityTds;
DFRobot_PH ph;
DFRobot_EC ec;

#define MOISTURE_PIN1 A0
#define MOISTURE_PIN2 A1
#define MOISTURE_PIN3 A2
#define TDS_PIN A3
#define PH_PIN A4
#define EC_PIN A5
#define LIGHT_PIN A6

#define BUTTON_PIN 2
#define DHT22_PIN 3
#define ECHOPIN 4
#define TRIGPIN 5

#define RELAY1 23 //pH up if pH is too low
#define RELAY2 22 //pH down if pH is too high

void setup() {
  Serial.begin(115200);
  espSerial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  gravityTds.setPin(1);
  gravityTds.setAref(5.0);
  gravityTds.setAdcRange(1024);
  gravityTds.begin();
  ph.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ECHOPIN, INPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  Wire.begin();
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
}

void loop() {

  buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW && lastButtonState == HIGH) {
    displayIndex = (displayIndex + 1) % 5; 
    lcd.clear();
    delay(100);
  }

  lastButtonState = buttonState;

  float moistVal = getMoistureVal();
  float tdsVal = getTDSVal();
  float phVal = getPHVal();
  float ecVal = getECVal();
  float humVal = getHumidityVal();
  float tempVal = getTemperatureVal();
  int distVal = getDistanceVal();
  int luxVal = getLuxVal();

  Serial.print("Moisture: ");
  Serial.print(moistVal);
  Serial.print("\tTDS: ");
  Serial.print(tdsVal);
  Serial.print(" ppm\t");
  Serial.print("pH level: ");
  Serial.print(phVal);
  Serial.print("\tEC: ");
  Serial.print(ecVal);
  Serial.print(" ms/cm\t");
  Serial.print("Humidity: ");
  Serial.print(humVal);
  Serial.print("\tTemp: ");
  Serial.print(tempVal);
  Serial.print("\t");
  Serial.print("Water Level: ");
  Serial.print(distVal);
  Serial.print(" cm\t");
  Serial.print("Light Intensity: ");
  Serial.println(luxVal);

  //sending the data from arduino to nodemcuv3
  String str = "Moisture: " + String(moistVal) + "\tTDS: " + String(tdsVal) + " ppm\t" +
             "pH level: " + String(phVal) + "\tEC: " + String(ecVal) + " ms/cm\t" +
             "Humidity: " + String(humVal) + "\tTemp: " + String(tempVal) + "\t" +
             "Water Level: " + String(distVal) + " cm\t" + "Light Intensity: " + String(luxVal);

  espSerial.println(str);

  if (displayIndex == 0) {
    lcd.setCursor(0, 0);
    lcd.print("SM: ");
    lcd.print(moistVal);
    lcd.setCursor(0, 1);
    lcd.print("TDS: ");
    lcd.print(tdsVal);
  }

  if (displayIndex == 1) {
    lcd.setCursor(0, 0);
    lcd.print("pH: ");
    lcd.print(phVal);
    lcd.setCursor(0, 1);
    lcd.print("EC: ");
    lcd.print(ecVal, 2);
    lcd.print(" ms/cm");
  }

  if (displayIndex == 2) {
    lcd.setCursor(0, 0);
    lcd.print("Hum: ");
    lcd.print(humVal);
    lcd.setCursor(0, 1);
    lcd.print("Temp: ");
    lcd.print(tempVal);
  }

  if (displayIndex == 3) {
    lcd.setCursor(0, 0);
    lcd.print("Water Lvl: ");
    lcd.print(distVal);
    lcd.print(" cm");
    lcd.setCursor(0, 1);
    lcd.print("Lux: ");
    lcd.print(luxVal);
  } 

  if (displayIndex == 4) {
    lcd.setCursor(0, 0);
    lcd.print("IN ETA: ");
    lcd.print(i);
    lcd.setCursor(0, 1);
    lcd.print("OUT ETA: ");
    lcd.print(j);
  }

  if (i == 300) {
    digitalWrite(RELAY2, LOW);
    startTime = millis();

    while (millis() - startTime < 120000) {

    }
    digitalWrite(RELAY2, HIGH);
  }

  if (j == 450) {
    digitalWrite(RELAY1, LOW);
    startTime = millis(); 
    while (millis() - startTime < 30000) {

    }
    digitalWrite(RELAY1, HIGH);
    i = 0;
    j = 0;
  }
  delay(1000);
  i++;
  j++;
}


int getMoistureVal() {
  int moistVal1 = analogRead(MOISTURE_PIN1);
  int moistVal2 = analogRead(MOISTURE_PIN2);
  int moistVal3 = analogRead(MOISTURE_PIN3);
  int avgMoistVal = (moistVal1 + moistVal2 + moistVal3) / 3;
  if (avgMoistVal > 0) {
    lastMoistVal = avgMoistVal;
  }
  return avgMoistVal;
}


float getTDSVal() {
  gravityTds.update();
  float tdsVal = gravityTds.getTdsValue() / 4.3; 
  if (tdsVal > 0) {
    lastTdsVal = tdsVal;
  }
  return tdsVal;
}

float getPHVal() {
    static unsigned long timepoint = millis();
    if (millis() - timepoint > 1000U) {
        timepoint = millis();
        float voltage = analogRead(PH_PIN) / 1024.0 * 5000;  
        float phVal = ph.readPH(voltage, 25); 
        if (phVal > 0) {
            lastPhVal = phVal;
            return phVal;
        } else {
            return lastPhVal;
        }
    } else {
        return lastPhVal;
    }
}


float getECVal() {
  float voltage = analogRead(EC_PIN) / 1024.0 * 5000;
  float ecVal = ec.readEC(voltage, 25);
  if (ecVal > 0) {
    lastEcVal = ecVal;
  }
  return ecVal;
}

float getHumidityVal() {
  int chk = DHT.read22(DHT22_PIN);
  float humVal = DHT.humidity;
  if (humVal > 0) {
    lastHumVal = humVal;
  }
  return humVal;
}

float getTemperatureVal() {
  int chk = DHT.read22(DHT22_PIN);
  float tempVal = DHT.temperature;
  if (tempVal > 0) {
    lastTempVal = tempVal;
  }
  return tempVal;
}

int getDistanceVal() {
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(TRIGPIN, LOW);  
  int distVal = pulseIn(ECHOPIN, HIGH)/ 58;

  if (distVal > 0) {
    lastDistVal = distVal;
  }
  return distVal;
}

int getLuxVal() {
  int luxVal = analogRead(LIGHT_PIN);
  if (luxVal > 1) {
    lastLuxVal = luxVal;
  }
  return luxVal;
}
