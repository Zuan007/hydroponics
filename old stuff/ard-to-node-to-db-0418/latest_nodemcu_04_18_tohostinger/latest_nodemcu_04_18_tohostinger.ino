#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

const char* ssid = "CMLagarto";
const char* password = "Boogiewonderland0906*";
IPAddress host(154, 41, 240, 103); // Change this to your MySQL server's IP address
const char* user = "u911537442_farmcup";
const char* password_mysql = "F4rmcup_p4ssw0rd";
const char* database = "u911537442_farmcup";

WiFiClient client;
MySQL_Connection conn((Client *)&client);

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
}

void loop() {
  // Read data from serial
  String data = "";
  while (Serial.available()) {
    char c = Serial.read();
    data += c;
    Serial.write(c); // Echo back to confirm the character was received
  }

  // Print received data for debugging
  Serial.println("Received data:");
  Serial.println(data);

  // Parse the received data
  float moisture, tds, pH, ec, humidity, temp, waterLevel;
  int lightIntensity;

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
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  char query[128];
  sprintf(query, "INSERT INTO sensordata (moisture, tds, pH, EC, ambient_light, temperature, humidity, waterlevel) VALUES ('%f', '%f', '%f', '%f', '%d', '%f', '%f', '%f')", moisture, tds, pH, ec, lightIntensity, temp, humidity, waterLevel);
  cur_mem->execute(query);
  delete cur_mem;

  delay(5000); // Wait for 5 seconds before reading the next data
}
