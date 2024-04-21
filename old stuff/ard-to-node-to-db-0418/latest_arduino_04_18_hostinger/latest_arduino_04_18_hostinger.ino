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
unsigned long debounceDelay = 50;
SoftwareSerial espSerial(5, 6); //rx, tx
String str; //isesend sa esp
LiquidCrystal_I2C lcd(0x27, 16, 2);
dht DHT;
GravityTDS gravityTds;
DFRobot_PH ph;
DFRobot_EC ec;

#define MOISTURE_PIN A0
#define TDS_PIN A1
#define PH_PIN A2
#define EC_PIN A3
#define LIGHT_PIN A4
#define BUTTON_PIN 2
#define DHT22_PIN 3
#define ECHOPIN 4
#define TRIGPIN 5
#define RELAY1 36
#define RELAY2 34
#define RELAY3 32
#define RELAY4 30
#define RELAY5 28
#define RELAY6 26
#define RELAY7 24

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
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  pinMode(RELAY5, OUTPUT);
  pinMode(RELAY6, OUTPUT);
  pinMode(RELAY7, OUTPUT);
  delay(2000);
}

void loop() {

  //change lcd screen
  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (buttonState == HIGH) {
      displayIndex = (displayIndex + 1) % 5;
      lcd.clear();
      delay(100);
    }
  }

  float moistVal = getMoistureVal();
  float tdsVal = getTDSVal();
  float phVal = getPHVal();
  float ecVal = getECVal();
  float humVal = getHumidityVal();
  float tempVal = getTemperatureVal();
  int distVal = getDistanceVal();
  int luxVal = getLuxVal();

  Serial.println(displayIndex); //debug
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
  Serial.print(luxVal);


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
  delay(5000);
}

int getMoistureVal() {
  int moistVal = analogRead(MOISTURE_PIN);
  return moistVal;
}

float getTDSVal() {
  gravityTds.update();
  float tdsVal = gravityTds.getTdsValue();
  return tdsVal;
}

float getPHVal() {
  float voltage = analogRead(PH_PIN) / 1024.0 * 5000;
  float phVal = ph.readPH(voltage, 25);
  return phVal;
}

float getECVal() {
  float voltage = analogRead(EC_PIN) / 1024.0 * 5000;
  float ecVal = ec.readEC(voltage, 25);
  return ecVal;
}

float getHumidityVal() {
  int chk = DHT.read22(DHT22_PIN);
  float humVal = DHT.humidity;
  return humVal;
}

float getTemperatureVal() {
  int chk = DHT.read22(DHT22_PIN);
  float tempVal = DHT.temperature;
  return tempVal;
}

int getDistanceVal() {
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(TRIGPIN, LOW); 
  int distVal = pulseIn(ECHOPIN, HIGH)/ 58;
  return distVal;
}

int getLuxVal() {
  int luxVal = analogRead(LIGHT_PIN);
  return luxVal;
}