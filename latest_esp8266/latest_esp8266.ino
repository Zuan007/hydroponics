#include <ESP8266WiFi.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

const char* ssid = "GlobeAtHome_892DF";
const char* password = "89763FD7";
IPAddress host(154, 41, 240, 103);
const char* user = "u911537442_farmcup";
const char* password_mysql = "F4rmcup_p4ssw0rd";
const char* database = "u911537442_farmcup";

#define RELAY1 D1 //pH up
#define RELAY2 D2 //pH down
#define RELAY3 D3 //snaps
#define RELAY4 D8 //fans
#define RELAY5 D5 //mixer
#define RELAY6 D6 //reservoir
#define RELAY7 D7 //downspouts
#define RELAY8 D4 //growlight

WiFiClient client;
MySQL_Connection conn((Client *)&client);

float moisture = -1, tds = -1, pH = -1, ec = -1, humidity = -1, temp = -1, waterLevel = -1;
int lightIntensity = -1;

void setup() {
  Serial.begin(115200);

  connectWiFi();
  connectMySQL();

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  pinMode(RELAY5, OUTPUT);
  pinMode(RELAY6, OUTPUT);
  pinMode(RELAY7, OUTPUT);
  pinMode(RELAY8, OUTPUT);

  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, HIGH);
  digitalWrite(RELAY5, HIGH);
  digitalWrite(RELAY6, HIGH);
  digitalWrite(RELAY7, HIGH);
  digitalWrite(RELAY8, HIGH);
}

int retrieveToggle();
float* retrieveFuzzyControls();
void useFuzzyControl(float* fuzzyValues);
float* retrieveManualControls();
void useManualControl(float* manualValues);
void sendSensorData(float moisture, float tds, float pH, float ec, float humidity, float temp, float waterLevel, int lightIntensity);

void loop() {
  bool validDataReceived = false;

  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    Serial.print("Received data: ");
    Serial.println(data);

    int index = data.indexOf("Moisture: ");
    if (index != -1) {
      moisture = data.substring(index + 10).toFloat();
    }

    index = data.indexOf("TDS: ");
    if (index != -1) {
      tds = data.substring(index + 5).toFloat();
    }

    index = data.indexOf("pH level: ");
    if (index != -1) {
      pH = data.substring(index + 9).toFloat();
    }

    index = data.indexOf("EC: ");
    if (index != -1) {
      ec = data.substring(index + 4).toFloat();
    }

    index = data.indexOf("Humidity: ");
    if (index != -1) {
      humidity = data.substring(index + 10).toFloat();
    }

    index = data.indexOf("Temp: ");
    if (index != -1) {
      temp = data.substring(index + 6).toFloat();
    }

    index = data.indexOf("Water Level: ");
    if (index != -1) {
      waterLevel = data.substring(index + 13).toFloat();
    }

    index = data.indexOf("Light Intensity: ");
    if (index != -1) {
      lightIntensity = data.substring(index + 17).toInt();
    }


    if (moisture != -1 && tds != -1 && pH != -1 && ec != -1 && humidity != -1 && temp != -1 && waterLevel != -1 && lightIntensity != -1) {
      validDataReceived = true; 
    }
  }

  if (validDataReceived) {
    float* fuzzyValues = retrieveFuzzyControls();
    float* manualValues = retrieveManualControls();

    if (!conn.connected()) {
      if (conn.connect(host, 3306, const_cast<char*>(user), const_cast<char*>(password_mysql), const_cast<char*>(database))) {
        Serial.println("Reconnected to MySQL Server.");
      } else {
        Serial.println("Reconnection failed.");
        return;
      }
    }

    int toggle = retrieveToggle();

    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
    char query[256];
    snprintf(query, sizeof(query), "INSERT INTO sensordata (moisture, tds, pH, EC, ambient_light, temperature, humidity, waterlevel) VALUES ('%.2f', '%.2f', '%.2f', '%.2f', '%d', '%.2f', '%.2f', '%.2f')", moisture, tds, pH, ec, lightIntensity, temp, humidity, waterLevel);
    cur_mem->execute(query);
    delete cur_mem;
    Serial.println("Data inserted into MySQL database.");

    if (toggle == 0) {
      useFuzzyControl(fuzzyValues);
    } else if (toggle == 1) {
      useManualControl(manualValues);
    }
  }

  delay(3000);
}

void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi!");
}

void connectMySQL() {
  Serial.print("Connecting to MySQL Server...");
  if (conn.connect(host, 3306, const_cast<char*>(user), const_cast<char*>(password_mysql), const_cast<char*>(database))) {
    Serial.println("Connected to MySQL Server!");
  } else {
    Serial.println("Connection failed.");
    return;
  }
}

int retrieveToggle() {
  MySQL_Cursor *cur_toggle = new MySQL_Cursor(&conn);

  char query[] = "SELECT toggle FROM toggle_switch ORDER BY time_stamp DESC LIMIT 1";
  cur_toggle->execute(query);

  row_values *row = NULL;
  int toggle = -1;
  column_names *columns = cur_toggle->get_columns();
  do {
    row = cur_toggle->get_next_row();
    if (row != NULL) {
      toggle = atoi(row->values[0]);
      Serial.print("Toggle value: ");
      Serial.println(toggle);
    }
  } while (row != NULL);
  delete cur_toggle;

  return toggle;
}

float* retrieveFuzzyControls() {
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);

  char query[] = "SELECT * FROM fuzzy_control ORDER BY time_stamp DESC LIMIT 1";
  cur_mem->execute(query);

  row_values *row = NULL;
  column_names *columns = cur_mem->get_columns();

  float* fuzzyValues = new float[columns->num_fields];

  for (int i = 0; i < columns->num_fields; i++) {
    fuzzyValues[i] = -1;
  }
  do {
    row = cur_mem->get_next_row();
    if (row != NULL) {
      for (int i = 0; i < columns->num_fields; i++) {
        fuzzyValues[i] = atof(row->values[i]);
        Serial.print(columns->fields[i]->name);
        Serial.print(": ");
        Serial.println(row->values[i]);
      }
    }
  } while (row != NULL);
  delete cur_mem;
  return fuzzyValues;
}

void useFuzzyControl(float* fuzzyValues) {

  digitalWrite(RELAY1, LOW);
  Serial.println(fuzzyValues[2]);
  Serial.println("Fuzzy: Adding pH Up");
  delay(fuzzyValues[2]);
  digitalWrite(RELAY1, HIGH);

  digitalWrite(RELAY2, LOW);
  Serial.println(fuzzyValues[3]);
  Serial.println("Fuzzy: Adding pH Down");
  delay(fuzzyValues[3]);
  digitalWrite(RELAY2, HIGH);

  digitalWrite(RELAY3, LOW);
  Serial.println(fuzzyValues[1]);
  Serial.println("Fuzzy: Adding Snaps A and B");
  delay(fuzzyValues[1]);
  digitalWrite(RELAY3, HIGH);

  digitalWrite(RELAY6, LOW);
  Serial.println(fuzzyValues[6]);
  Serial.println("Fuzzy: Adding Water to Tank");
  delay(fuzzyValues[6]);
  digitalWrite(RELAY6, HIGH);

  digitalWrite(RELAY4, LOW);
  Serial.println(fuzzyValues[7]);
  Serial.println("Fuzzy: Fans are working");
  delay(fuzzyValues[7]);
  digitalWrite(RELAY4, HIGH);

  digitalWrite(RELAY8, LOW);
  Serial.println(fuzzyValues[8]);
  Serial.println("Fuzzy: Light Relay is working");
  delay(fuzzyValues[8]);
  digitalWrite(RELAY8, HIGH);  

  digitalWrite(RELAY6, HIGH);
  Serial.println(fuzzyValues[5]);
  Serial.println("Fuzzy: Water Pump Relay is working");
  delay(fuzzyValues[5]);
  digitalWrite(RELAY6, LOW);

  digitalWrite(RELAY7, HIGH);
  Serial.println(fuzzyValues[4]);
  Serial.println("Fuzzy: Downspout Relay is working");
  delay(fuzzyValues[4]);
  digitalWrite(RELAY7, LOW);

}

float* retrieveManualControls() {
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);

  char query[] = "SELECT * FROM manual_control ORDER BY time_stamp DESC LIMIT 1";
  cur_mem->execute(query);

  row_values *row = NULL;
  column_names *columns = cur_mem->get_columns(); 

  float* manualValues = new float[columns->num_fields];

  for (int i = 0; i < columns->num_fields; i++) {
    manualValues[i] = -1;
  }

  do {
    row = cur_mem->get_next_row();
    if (row != NULL) {
      for (int i = 0; i < columns->num_fields; i++) {
        manualValues[i] = atof(row->values[i]);
        Serial.print(columns->fields[i]->name);
        Serial.print(": ");
        Serial.println(row->values[i]);
      }
    }
  } while (row != NULL);

  delete cur_mem;

  return manualValues;
}

void useManualControl(float* manualValues) {
  digitalWrite(RELAY1, LOW);
  Serial.println(manualValues[1]);
  Serial.println("Manual: Adding pH Up");
  delay(manualValues[1]);
  digitalWrite(RELAY1, HIGH);

  digitalWrite(RELAY2, LOW);
  Serial.println(manualValues[2]);
  Serial.println("Manual: Adding pH Down");
  delay(manualValues[2]);
  digitalWrite(RELAY2, HIGH);

  digitalWrite(RELAY3, LOW);
  Serial.println(manualValues[3]);
  Serial.println("Manual: Snap A and B Relay is working");
  delay(manualValues[3]);
  digitalWrite(RELAY3, HIGH);

  digitalWrite(RELAY4, LOW);
  Serial.println(manualValues[4]);
  Serial.println("Manual: Fans are working");
  delay(manualValues[4]);
  digitalWrite(RELAY4, HIGH);

  digitalWrite(RELAY5, LOW);
  Serial.println(manualValues[5]);
  Serial.println("Manual: Mixing Relay is working");
  delay(manualValues[5]);
  digitalWrite(RELAY5, HIGH);

  digitalWrite(RELAY6, LOW);
  Serial.println(manualValues[6]);
  Serial.println("Manual: Adding water to reservoir!");
  delay(manualValues[6]);
  digitalWrite(RELAY6, HIGH);

  digitalWrite(RELAY7, LOW);
  Serial.println(manualValues[7]);
  Serial.println("Manual: Adding water to downspouts!");
  delay(manualValues[7]);
  digitalWrite(RELAY7, HIGH);

  digitalWrite(RELAY8, LOW);
  Serial.println(manualValues[8]);
  Serial.println("Manual: Turning on lights");
  delay(manualValues[8]);
  digitalWrite(RELAY8, HIGH);

}

void sendSensorData(float moisture, float tds, float pH, float ec, float humidity, float temp, float waterLevel, int lightIntensity) {
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  char query[256];
  snprintf(query, sizeof(query), "INSERT INTO sensordata (moisture, tds, pH, EC, ambient_light, temperature, humidity, waterlevel) VALUES ('%.2f', '%.2f', '%.2f', '%.2f', '%d', '%.2f', '%.2f', '%.2f')", moisture, tds, pH, ec, lightIntensity, temp, humidity, waterLevel);
  if (!cur_mem->execute(query)) {
    Serial.println("Error inserting data into MySQL database.");
  } else {
    Serial.println("Data inserted into MySQL database.");
  }

  delete cur_mem;
}
