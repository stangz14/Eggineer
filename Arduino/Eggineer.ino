#include <LiquidCrystal_I2C.h>
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include "RTC.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <qrcode.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int btn_confirm = 8;
int btn_cancel = 9;
int btn_1 = 12;
int btn_2 = 11;
int btn_3 = 10;

int state = 1;
// state 1 = เครื่องไม่ได้ทำงาน
// state 2 = เครื่องรอการยืนยัน
// state 3 = เครื่องกำลังทำงาน

int option = 0;

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

unsigned long lastTick = 0; 
const unsigned long tickInterval = 1000;

const char ssid[] = ""; // WiFi SSID
const char pass[] = ""; // WiFi Password

LiquidCrystal_I2C lcd(0x27, 16, 2);

const char serverAddress[] = ""; // Server URL or IP
const int port = 80;
WiFiClient wifi;

HttpClient client = HttpClient(wifi, serverAddress, port);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600);

StaticJsonDocument<256> responseDoc;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

String token = "";


void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;

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
  pinMode(btn_3, INPUT);

  lcd.init();
  lcd.backlight();
  lcdDisplay();

  if (!display.begin(0x3C, true)) {
    Serial.println(F("SH1106 allocation failed"));
    for (;;)
      ;
  }

  display.clearDisplay();
  display.display();
  sensors.begin();
}

void loop() {
  checkButton();
  sensors.requestTemperatures();

  lcdDisplay();
  delay(500);
}

void checkButton() {
  if (digitalRead(btn_3) == HIGH) {
    // Cooked Egg
    option = 1;
    state = 2;
    temp_eggsuccess = 90;
    diameter = 45;
    type_egg = "Cooked";
    lcdDisplay();
  } else if (digitalRead(btn_2) == HIGH) {
    option = 2;
    state = 2;
    temp_eggsuccess = 82;
    diameter = 45;
    type_egg = "bael eggs";
    lcdDisplay();
  } else if (digitalRead(btn_3) == HIGH) {
    option = 3;
    state = 2;
    temp_eggsuccess = 73;
    diameter = 45;
    type_egg = "Soft-boiled";
    lcdDisplay();
  } else if (digitalRead(btn_confirm) == HIGH && state == 2) {
    state = 3;
    option = 0;
    lcd.clear();
    lcd.print("Loading...");
    calculateTime();
    lcd.clear();
    lcdDisplay();
  } else if (digitalRead(btn_cancel) == HIGH && state == 2) {
    lcd.clear();
    lcd.print("    CANCEL!!      ");
    lcd.setCursor(0, 1);
    lcd.print("    CANCEL!!      ");
    delay(1000);
    state = 1;
    lcdDisplay();
  }
}

void lcdDisplay() {
  lcd.clear();
  lcd.home();

  if (state == 1) {
    lcd.print("Choose egg type");
    float tempC = sensors.getTempCByIndex(0);
    if (state == 1) {
      lcd.setCursor(0, 1);
      lcd.print(tempC);
      lcd.print(" ");
      lcd.print((char)223);
      lcd.print("C");
    }
    return;
  }

  if (state == 2) {
    if (option == 1) {
      lcd.print("cooked eggs");
      lcd.setCursor(0, 1);
      lcd.print("Are you sure Y/N");
    } else if (option == 2) {
      lcd.print("bael eggs");
      lcd.setCursor(0, 1);
      lcd.print("Are you sure Y/N");
    } else if (option == 3) {
      lcd.print("soft-boiled eggs");
      lcd.setCursor(0, 1);
      lcd.print("Are you sure Y/N");
    }
    return;
  }

  
  unsigned long now = millis();
  if (now - lastTick >= 1000) {
    lastTick = now;
    // ลดเวลา 1 วิ
    sec--;
    if (sec < 0) {
      if (miin > 0) {
        miin -= 1;
        sec = 59;
      } else {
        // ถึง 0:00
        sec = 0;
      }
    }
  }

  // แสดงเวลา
  lcd.setCursor(0,0);
  lcd.print("Time :");
  if (miin < 10) {
    lcd.print("0");
    lcd.print(miin);
  } else {
    lcd.print(miin);
  }
  lcd.print(":");
  if (sec < 10) {
    lcd.print("0");
    lcd.print(sec);
  } else {
    lcd.print(sec);
  }

  if (miin == 0 && sec == 0) {
    lcd.clear();
    lcd.home();
    lcd.print(" YOUR EGG READY");
    state = 1;
    display.clearDisplay();
    display.display();

    String postData = "{\"token\":\"" + token + "\"}";

    if (makePostRequest("/sendsuccess", postData, responseDoc)){
      const char* data = responseDoc["status"];
      Serial.println("\n--- Parsed POST Response ---");
      Serial.print("Status: "); Serial.println(data);
    } else {
      Serial.println("POST request failed.");
    }
    delay(2000);
  }
}

void calculateTime() {
  waterTemp = sensors.getTempCByIndex(0);
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
    lcdDisplay();
    return;
  }

  tempWE = 2 * (waterTemp - 25);
  tempWS = waterTemp - temp_eggsuccess;

  temp = 0.0015 * (pow(diameter, 2) * log(tempWE / tempWS));
  secTemp = (temp - (int)temp) * 60;

  sec = int(secTemp);
  miin = (int)temp;

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

  String postData = "{\"type\":\"" + type_egg + "\",\"start_time\":\"" + start_hour + "." + start_minute + "." + start_minute + "\",\"end_time\":\"" + end_hour + "." + end_minute + "." + end_minute + "\"}";

  Serial.println(postData);
  if (makePostRequest("/createtoken", postData, responseDoc)) {
    const char* data = responseDoc["token"];

    Serial.println("\n--- Parsed POST Response ---");
    Serial.print("Token: ");
    Serial.println(data);
    token = data;
    makeQRcode(data);
  } else {
    Serial.println("POST request failed.");
  }
}

bool makePostRequest(const char* path, const String& body, JsonDocument& doc) {
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

void makeQRcode(const char* token) {
  display.clearDisplay();
  display.setContrast(0x10);

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 2, ECC_LOW, token);

  int qrSize = qrcode.size;
  int pixelSize = min(SCREEN_WIDTH / qrcode.size, SCREEN_HEIGHT / qrcode.size);
  int xOffset = (SCREEN_WIDTH - qrSize * pixelSize) / 2;
  int yOffset = (SCREEN_HEIGHT - qrSize * pixelSize) / 2;
  // Draw QR code
  for (int y = 0; y < qrSize; y++) {
    for (int x = 0; x < qrSize; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        display.fillRect(xOffset + x * pixelSize, yOffset + y * pixelSize,
                         pixelSize, pixelSize, SH110X_WHITE);
      }
    }
  }

  display.display();
}
