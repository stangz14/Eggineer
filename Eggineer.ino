#include <LiquidCrystal_I2C.h>
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>   // <-- Add this
#include "RTC.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

int btn_confirm = 8;
int btn_cancel = 9;
int btn_1 = 10;
int btn_2 = 11;

int state = 1;
// state 1 = เครื่องไม่ได้ทำงาน
// state 2 = เครื่องรอการยืนยัน
// state 3 = เครื่องกำลังทำงาน

int option = 0;
// option 0 ยังไม่ได้เลือกชนิดการต้มไข่
String type_egg = "";

float waterTemp = 0;
float temp_eggsuccess = 0;
float diameter = 0;
float tempWE = 0;
float tempWS = 0;
float temp = 0;
float secTemp = 0;

int miin = 0;
int sec = 0;

unsigned long previousMillis = 0;
const long interval = 1000;

// --- Your network credentials ---
const char ssid[] = "Muhaha";
const char pass[] = "12345678";

LiquidCrystal_I2C lcd(0x27, 16, 2);

const char serverAddress[] = "tough-singers-design.loca.lt";
const int port = 80;
WiFiClient wifi;

HttpClient client = HttpClient(wifi, serverAddress, port);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600);

StaticJsonDocument<256> responseDoc;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial);

  // --- Connect to WiFi ---
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(2000);
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();

  RTC.begin();

  pinMode(btn_confirm, INPUT);
  pinMode(btn_cancel, INPUT);
  pinMode(btn_1, INPUT);
  pinMode(btn_2, INPUT);

  lcd.init();
  lcd.backlight();
  display();
}

void loop() {
  // put your main code here, to run repeatedly:
  checkButton();
}

void checkButton() {
  if (digitalRead(btn_1) == HIGH) {
    // Cooked Egg
    option = 1;
    state = 2;
    temp_eggsuccess = 90;
    diameter = 42;
    type_egg = "Cooked";
    display();
  } else if (digitalRead(btn_2) == HIGH) {
    option = 2;
    state = 2;
    display();
  } else if (digitalRead(btn_confirm) == HIGH && state == 2) {
    state = 3;
    option = 0;
    calculateTime();
    display();
  } else if (digitalRead(btn_cancel) == HIGH && state == 2) {
    lcd.clear();
    lcd.print("    CANCEL!!      ");
    lcd.setCursor(0, 1);
    lcd.print("    CANCEL!!      ");
    delay(1000);
    state = 1;
    display();
  }
}

void display() {
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= interval) {
    lcd.clear();
    previousMillis = currentMillis;
    if (state == 3){
      sec -= 1;
    }
  }
  lcd.home();
  if (state == 1) {
    lcd.print("Choose egg type");
  } else if (state == 2 && option == 1) {
    lcd.print("cooked eggs");
    lcd.setCursor(0, 1);
    lcd.print("Are you sure Y/N");

  } else if (state == 2 && option == 2) {
    lcd.print("soft-boiled eggs");
    lcd.setCursor(0, 1);
    lcd.print("Are you sure Y/N");
  } else if (state == 3) {
    if (sec < 0 && miin != 0){
      sec = 59;
      miin -= 1;
    }

    if (miin < 10){
      lcd.setCursor(8, 1);
      lcd.print(miin);
      lcd.setCursor(7, 1);
      lcd.print("0");
    } else {
      lcd.setCursor(7, 1);
      lcd.print(miin);
    }
    lcd.setCursor(9, 1);
    lcd.print(":");
    if (sec < 10){
      lcd.setCursor(11, 1);
      lcd.print(sec);
      lcd.setCursor(10, 1);
      lcd.print("0");
    } else {
      lcd.setCursor(10, 1);
      lcd.print(sec);
    }

    if (sec == 0 && miin == 0){
      lcd.clear();
      lcd.home();
      lcd.print(" YOUR EGG READY");
      state = 1;
      delay(2000);
    }
    display();
  }
}

void calculateTime() {
  // TEMP
  waterTemp = 100;
  if (waterTemp < temp_eggsuccess) {
    lcd.clear();
    lcd.home();
    lcd.print("WATER TEMPURETER");
    lcd.setCursor(0, 1);
    lcd.print(" WASN'T  ENOUGH");
    delay(1000);
    lcd.clear();
    option = 0;
    state = 1;
    display();
    return;
  }

  tempWE = 2 * (waterTemp - 25);
  tempWS = waterTemp - temp_eggsuccess;

  temp = 0.0015 * (pow(diameter, 2) * log(tempWE / tempWS));
  secTemp = (temp - (int)temp) * 60;

  // TEMP
  // sec = int(secTemp);
  sec = 10;
  // miin = (int)temp;
  miin = 0;

  Serial.print(miin);
  Serial.print(":");
  Serial.println(sec);

  timeClient.update();

  int start_hour = timeClient.getHours();
  int start_minute = timeClient.getMinutes();
  int start_second = timeClient.getSeconds();

  int end_secound = start_second + sec;
  int end_minute = start_minute + miin;
  int end_hour = start_hour;

  end_minute += end_secound / 60;
  end_secound = end_secound % 60;
  end_hour += end_minute / 60;
  end_minute = end_minute % 60;
  end_hour = end_hour % 24;

  String postData = "{\"type\":\"" + type_egg +
                    "\",\"start_time\":\"" + start_hour + ":" + start_minute + ":" + start_minute +
                    "\",\"end_time\":\"" + end_hour + ":" + end_minute + ":" + end_minute +"\"}";

  Serial.println(postData);
  if (makePostRequest("/createtoken", postData, responseDoc)){
    const char* status = responseDoc["status"];
    const char* message = responseDoc["message"];
    const char* data = responseDoc["data_received"]["token"];

    Serial.println("\n--- Parsed POST Response ---");
    Serial.print("Status: "); Serial.println(status);
    Serial.print("Message: "); Serial.println(message);
    Serial.print("Token: "); Serial.println(data);
  } else {
    Serial.println("POST request failed.");
  }
}

bool makePostRequest(const char* path, const String& body, JsonDocument& doc){
  Serial.print("\nMaking POST request to: ");
  Serial.println(path);

  client.beginRequest();
  client.post(path);
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Accept", "application/json");  // บาง API ต้องการ
  client.sendHeader("Content-Length", body.length());
  client.beginBody();
  client.print(body);
  client.endRequest();

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);

  if (statusCode != 200 && statusCode != 201) {
    Serial.println("HTTP Error!");
    Serial.println(response);
    return false;
  }

  // แปลง String response ให้เป็น JSON
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }
  return true;
}
