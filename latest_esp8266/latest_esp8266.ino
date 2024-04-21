#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

const char* ssid = ""; // Enter your network SSID
const char* password = ""; // Enter your network password
IPAddress host(154, 41, 240, 103); // Change this to your MySQL server's IP address
const char* user = "u911537442_farmcup";
const char* password_mysql = "F4rmcup_p4ssw0rd";
const char* database = "u911537442_farmcup";

WiFiClient client;
MySQL_Connection conn((Client *)&client);

#define RELAY1 D0 //snap a and b for ec and tds
#define RELAY2 D1 //pH up if pH is too low
#define RELAY3 D2 //pH down if pH is too high
#define RELAY4 D3 //water pump for downspouts
#define RELAY5 D4 //water pump for the water container
#define RELAY6 D5 //water pump for mixing
#define RELAY7 D6 //fans for temp and humidity
#define RELAY8 D7 //growlight for light level control (pwede ding timed)
#define RELAY9 D8 //water pump for sump container (in)
#define RELAY10 A0 //water pump back to water container (out)

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  Serial.println("Connected to WiFi");

  Serial.println("Connecting to MySQL Server...");
  if (conn.connect(host, 3306, const_cast<char*>(user), const_cast<char*>(password_mysql), const_cast<char*>(database))) {
    Serial.println("MySQL Connected!");
  } else {
    Serial.println("Connection failed.");
    return;
  }

  //set relay pins as output
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  pinMode(RELAY5, OUTPUT);
  pinMode(RELAY6, OUTPUT);
  pinMode(RELAY7, OUTPUT);
  pinMode(RELAY8, OUTPUT);
  pinMode(RELAY9, OUTPUT);
  pinMode(RELAY10, OUTPUT);
}

void loop() {
  // Read data from serial
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    Serial.print("Received data: ");
    Serial.println(data);

    // Parse the received data
    float moisture = -1, tds = -1, pH = -1, ec = -1, humidity = -1, temp = -1, waterLevel = -1;
    int lightIntensity = -1;

    int index = data.indexOf("Moisture: ");
    if (index != -1) {
      moisture = data.substring(index + 10, data.indexOf("\t", index)).toFloat();
    }

    index = data.indexOf("TDS: ");
    if (index != -1) {
      tds = data.substring(index + 5, data.indexOf(" ppm", index)).toFloat();
    }

    index = data.indexOf("pH level: ");
    if (index != -1) {
      pH = data.substring(index + 9, data.indexOf("\t", index)).toFloat();
    }

    index = data.indexOf("EC: ");
    if (index != -1) {
      ec = data.substring(index + 4, data.indexOf(" ms/cm", index)).toFloat();
    }

    index = data.indexOf("Humidity: ");
    if (index != -1) {
      humidity = data.substring(index + 10, data.indexOf("\t", index)).toFloat();
    }

    index = data.indexOf("Temp: ");
    if (index != -1) {
      temp = data.substring(index + 6, data.indexOf("\t", index)).toFloat();
    }

    index = data.indexOf("Water Level: ");
    if (index != -1) {
      waterLevel = data.substring(index + 13, data.indexOf(" cm", index)).toFloat();
    }

    index = data.indexOf("Light Intensity: ");
    if (index != -1) {
      lightIntensity = data.substring(index + 17, data.indexOf("\t", index)).toInt();
    }

    // Insert sensor data into MySQL database
    if (moisture != -1 && tds != -1 && pH != -1 && ec != -1 && humidity != -1 && temp != -1 && waterLevel != -1 && lightIntensity != -1) {
      MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
      char query[256];
      snprintf(query, sizeof(query), "INSERT INTO sensordata (moisture, tds, pH, EC, ambient_light, temperature, humidity, waterlevel) VALUES ('%.2f', '%.2f', '%.2f', '%.2f', '%d', '%.2f', '%.2f', '%.2f')", moisture, tds, pH, ec, lightIntensity, temp, humidity, waterLevel);
      cur_mem->execute(query);
      delete cur_mem;
      Serial.println("Data inserted into MySQL database.");
    
      // Sample relay control logic
      if (moisture <= 300) {
        digitalWrite(RELAY4, HIGH); // Turn on relay for water pump
      } else {
        digitalWrite(RELAY4, LOW); // Turn off relay for water pump
      }

      if (tds >= 500) {
        digitalWrite(RELAY1, HIGH); // Turn on relay for snap a & b
      } else {
        digitalWrite(RELAY1, LOW); // Turn off relay for snap a & b
      }
    } else {
      Serial.println("Error: Invalid sensor data.");
    }
  }

  delay(5000); // Wait for 5 seconds before reading the next data (adjust as you please!)
}
