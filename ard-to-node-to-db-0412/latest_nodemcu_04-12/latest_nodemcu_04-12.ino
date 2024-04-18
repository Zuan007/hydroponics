#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "CMLagarto";
const char* password = "Boogiewonderland0906*";
const char* serverName = "http://192.168.1.13/hydroponics/post-esp-data.php";
String apiKeyValue = "tPmAT5Ab3j7F9";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  Serial.println("Connected to WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    if (http.begin(client, serverName)) {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

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
      float moisture = getValueFloat(data, "Moisture: ", "\t");
      float tds = getValueFloat(data, "TDS: ", " ppm\t");
      float pH = getValueFloat(data, "pH level: ", "\t");
      float ec = getValueFloat(data, "EC: ", " ms/cm\t");
      float humidity = getValueFloat(data, "Humidity: ", "\t");
      float temp = getValueFloat(data, "Temp: ", "\t");
      float waterLevel = getValueFloat(data, "Water Level: ", " cm\t");
      int lightIntensity = getValue(data, "Light Intensity: ", "\t");

      // Prepare the HTTP request data
      String httpRequestData = "api_key=" + apiKeyValue +
                                "&soil_moisture=" + String(moisture) +
                                "&total_dissolved_solids=" + String(tds) +
                                "&pH_level=" + String(pH) +
                                "&EC_level=" + String(ec) +
                                "&light_level=" + String(lightIntensity) +
                                "&temperature=" + String(temp) +
                                "&humidity=" + String(humidity) +
                                "&water_level=" + String(waterLevel);

      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
      } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }

      http.end();
    } else {
      Serial.println("Failed to connect to server");
    }
  } else {
    Serial.println("WiFi Disconnected");
  }

  delay(5000); // Send an HTTP POST request every 5 seconds
}


float getValueFloat(String data, String key, String delimiter) {
  int index = data.indexOf(key) + key.length();
  int endIndex = data.indexOf(delimiter, index);
  String valueStr = data.substring(index, endIndex);
  return valueStr.toFloat();
}

int getValue(String data, String key, String delimiter) {
  int index = data.indexOf(key) + key.length();
  int endIndex = data.indexOf(delimiter, index);
  String valueStr = data.substring(index, endIndex);
  return valueStr.toInt();
}