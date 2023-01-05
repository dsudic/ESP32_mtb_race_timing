#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include<SD.h>

#define SS_PIN 5       // Chip Select pin for RFID reader
#define RST_PIN 22     // RFID reader RST pin
#define SD_CS 15       // Chip Select pin for SD card reader
#define BUFF_LENGTH 10 // Size of an array for temporary storing finish times

const char* ssid     = "Finish";
const char* password = "timingXyZ";

const char* serverName = "http://pzi1.fesb.hr/~dsudic19/Timing/handle_post_from_finish.php"; //http://pzi1.fesb.hr/~dsudic19/test/php/handle-form.php  http://pzi1.fesb.hr/~dsudic19/Timing-server/php/handle_post.php

String apiKeyValue = "******";

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;
byte prevUID[4];               // Init array that will store new NUID
String UID = "";

int buttonSync = 27;           // Button for sync on D1
int prevSyncState = LOW;
int finishSensor = 13;         // Light sensor on D2
int beamIndicator = 21;        // LED indicator on D4
unsigned long finishSyncTime;
unsigned long finish = 0;
unsigned long interval = 1000; // Interval in which sensor is not sensing after the beam is broken
int previousState = 0;

int prevReconnState = LOW;
int reconn = 25; // Button for reconnecting to the network

unsigned long buff[BUFF_LENGTH] = {0}; // Array for temporary storing the finish times to avoid needing the rider to scan his RFID quickly before new a rider crosses the finish line
int in = 0;                            // Index where new value will be stored in an array
int out = 0;                           // Finish time from this index will be sent next to the server/SD card
const int resetIndexes = 32;           // Button for reseting buffer indexes (in, out) on pin 32
int prevStateRI = HIGH;
int currentStateRI;
unsigned long pressedTimeRI = 0;
bool pressingRI = false;
bool longDetectedRI = false;

const int falseFinish = 33; // Button for overwriting last value in buffer in case someone crossed finish accidentally
int prevStateFF = HIGH;
int currentStateFF;
unsigned long pressedTimeFF = 0;
bool pressingFF = false;
bool longDetectedFF = false;

boolean new_tag = false;

// Buzzer
int buzzer = 26;
int freq = 3000;
int channel = 0;
int resolution = 8;
unsigned long previousMillis = 0; // Last time buzzer changed state
unsigned long buzzerOn = 100;
unsigned long buzzerOff = 10;
boolean buzz_flag = true;         // Permission to call RFIDScanSound function (!buzz_flag)
int counter = 0;                  // In which phase is beeping (On(100ms)-Off(10ms)-On(100ms)-Off)
unsigned long lastBuzz = 0;       // PrevMillis for buzzer indicator for beam and buttons

String SDlogData = "";
File SDfile;

int WiFiLED = 2;

void setup() {
  Serial.begin(115200); // Arduino 9600

  pinMode(buttonSync, INPUT);
  pinMode(reconn, INPUT);
  pinMode(finishSensor, INPUT);
  pinMode(beamIndicator, OUTPUT);
  pinMode(resetIndexes, INPUT);
  pinMode(falseFinish, INPUT);
  pinMode(WiFiLED, OUTPUT);

  ledcSetup(channel, freq, resolution); // Buzzer
  ledcAttachPin(buzzer, channel);       // Buzzer

  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialization done!");

  //  uint8_t cardType = SD.cardType();
  //  if (cardType == CARD_NONE) {
  //    Serial.println("No SD card attached!");
  //    return;
  //  }

  SDfile = SD.open("/finish.txt", FILE_APPEND);

  if (SDfile) {
    SDfile.println("-----------------------------------------------");
    SDfile.println("Chip_ID       |   Finish[ms]");
    SDfile.println("");
    SDfile.close();
    Serial.println("Header line write done");
  }
  else {
    Serial.println("ERROR opening file");
  }

  //WiFi.onEvent(ReconnectWiFi, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.begin(ssid, password);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  //  Serial.println("Connecting");
  //  while (WiFi.status() != WL_CONNECTED) {
  //    delay(500);
  //    Serial.print(".");
  //  }
  //  Serial.println("");
  //  Serial.print("Connected to WiFi network with IP Address: ");
  //  Serial.println(WiFi.localIP());

  if (WiFi.status() == WL_CONNECTED)
    Serial.println(WiFi.localIP());
  else Serial.println("Not connected!");

}

void sendRequest(unsigned int finishTotal) {
  // Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);

    // Content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // HTTP POST request data to be send
    String httpRequestData = "api_key=" + apiKeyValue + "&finish=" + finishTotal + "&chip_id=" + UID;

    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);
    Serial.println("---------------------------------");

    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }

  else {
    Serial.println("WiFi Disconnected");
  }
}

void stringifyUID(byte *buffer, byte bufferSize) {
  UID = "";
  // Format UID string "XX XX XX XX"
  for (byte i = 0; i < bufferSize; i++) {
    UID += (buffer[i] < 0x16 ? "0" : "") + String(buffer[i], HEX) + (i != 3 ? " " : "");
  }
  UID.toUpperCase();
}

void getUID() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  // Check if it is the same tag as previously scanned
  if (rfid.uid.uidByte[0] != prevUID[0] ||
      rfid.uid.uidByte[1] != prevUID[1] ||
      rfid.uid.uidByte[2] != prevUID[2] ||
      rfid.uid.uidByte[3] != prevUID[3]) {
    Serial.println(F("A new card has been detected."));

    new_tag = true;

    // Turn a buzzer On
    ledcWriteTone(channel, 3000);
    // Mark when a buzzer started
    previousMillis = millis();
    buzz_flag = false;

    // Store NUID into prevUID array
    for (byte i = 0; i < 4; i++) {
      prevUID[i] = rfid.uid.uidByte[i];
    }

    Serial.print(F("The NUID tag is:"));
    stringifyUID(rfid.uid.uidByte, rfid.uid.size);

    Serial.print(UID);
    Serial.println();
  }
  else {
    //new_tag = false;
    Serial.println(F("Tag read previously."));
  }

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

void RFIDScanSound() {
  Serial.println("Sound");

  if (millis() - previousMillis >= buzzerOn && counter == 0) {
    ledcWrite(channel, 0);
    previousMillis = millis();
    counter++;
  }

  if ((millis() - previousMillis) >= buzzerOff && counter == 1) {
    ledcWriteTone(channel, 3000);
    previousMillis = millis();
    counter++;
  }

  if ((millis() - previousMillis) > buzzerOn && counter == 2) {
    ledcWrite(channel, 0);
    buzz_flag = true;
    counter = 0;
  }
}

void log2SD(unsigned int finishTotal) {
  SDlogData = UID + "   |   " + String(finishTotal);
  SDfile = SD.open("/finish.txt", FILE_APPEND);

  if (SDfile) {
    SDfile.println(SDlogData);
    SDfile.close();
    Serial.println("SD card written");
  }
  else
    Serial.println("ERROR opening file");
}

//---------------------------------------------------------------------------------------------
void loop() {

  int reconnState = digitalRead(reconn);
  if (reconnState == HIGH && prevReconnState == LOW) {
    if (WiFi.status() != WL_CONNECTED) {

      Serial.println("Connecting...");
      WiFi.begin(ssid, password);
      int WLcount = 0;
      while (WiFi.status() != WL_CONNECTED && WLcount < 200) {
        delay(100);
        //Serial.printf( "\tSSID = %s\n"   , WiFi.SSID().c_str() );
        //Serial.printf( "\tPSK  = %s\n\n" , WiFi.psk().c_str()  );
        Serial.print(".");
        ++WLcount;
      }
    }
  }
  prevReconnState = reconnState;

  digitalWrite(WiFiLED, (WiFi.status() != WL_CONNECTED) ? HIGH : LOW);

  // To reset IN and OUT indexes for buff array, press and hold button for 1.5s
  currentStateRI = digitalRead(resetIndexes);

  if (prevStateRI == LOW && currentStateRI == HIGH) {
    pressedTimeRI = millis();
    pressingRI = true;
    longDetectedRI = false;
  }
  else if (prevStateRI == HIGH && currentStateRI == LOW)
    pressingRI = false;

  if (pressingRI == true && longDetectedRI == false)
    if (millis() - pressedTimeRI > 1500) {
      in = 0;
      out = 0;
      Serial.print(in);
      Serial.println(out);
      longDetectedRI = true;
      ledcWrite(channel, 3000);
      lastBuzz = millis();
    }
  prevStateRI = currentStateRI;

  if (millis() - lastBuzz >= 400 && longDetectedRI)
    ledcWrite(channel, 0);

  // In case of a false beam break, press button for 1.5s to overwrite last value in a buff array
  currentStateFF = digitalRead(falseFinish);

  if (prevStateFF == LOW && currentStateFF == HIGH) {
    pressedTimeFF = millis();
    pressingFF = true;
    longDetectedFF = false;
  }
  else if (prevStateFF == HIGH && currentStateFF == LOW)
    pressingFF = false;
  if (pressingFF == true && longDetectedFF == false)
    if (millis() - pressedTimeFF > 1500) {
      in--;
      if (in < 0)
        in = 0;
      longDetectedFF = true;
      // If the button is pressed for 1.5s turn the buzzer on
      ledcWrite(channel, 3000);
      lastBuzz = millis();
    }
  prevStateFF = currentStateFF;

  // Turn the buzzer off after 400ms
  if (millis() - lastBuzz >= 400 && longDetectedFF)
    ledcWrite(channel, 0);

  int buttonState = digitalRead(buttonSync);
  if (buttonState == HIGH && prevSyncState == LOW) {
    finishSyncTime = millis();
    Serial.print("sync time on finish gate = ");
    Serial.println(finishSyncTime);

    // Log sync times on sd card
    String SDlog = "sync:" + String(finishSyncTime);
    SDfile = SD.open("/log_f.txt", FILE_APPEND);
    if (SDfile) {
      SDfile.println(SDlog);
      SDfile.close();
      Serial.println("Sync time written to SD card");
    }
    else
      Serial.println("ERROR opening file");
  }
  prevSyncState = buttonState;

  int sensorState_finish = digitalRead(finishSensor);
  // If beam is broken, turn LED on
  digitalWrite(beamIndicator, !sensorState_finish);
  //digitalWrite(14, !sensorState_finish); // testing w/ stopwatch

  if (sensorState_finish == LOW && sensorState_finish != previousState && (millis() - finish) > interval) { // To avoid multiple beam interruptions as the bicycle passes trough finish line
    finish = millis();
    Serial.print("finish time: ");
    Serial.println(finish);
    Serial.print("--> Real finish time (finish - sync): ");
    Serial.println(finish - finishSyncTime);
    //sendRequest();
    //delay(1000);
    ledcWrite(channel, 3000);
    lastBuzz = millis();

    if (in == BUFF_LENGTH)
      in = 0;
    buff[in++] = finish - finishSyncTime; // Temporary store the finish time in an array

    //    for (int i = 0; i < BUFF_LENGTH; i++)  // For watching changes in the buffer
    //      Serial.println(buff[i]);

    String SDlog2 = "break:" + String(finish - finishSyncTime);
    SDfile = SD.open("/log_f.txt", FILE_APPEND);

    if (SDfile) {
      SDfile.println(SDlog2);
      SDfile.close();
      Serial.println("Sync time written to SD card");
    }
    else
      Serial.println("ERROR opening file");
  }
  previousState = sensorState_finish;

  if (millis() - lastBuzz > 700)
    ledcWrite(channel, 0);

  getUID();

  if (!buzz_flag) {
    RFIDScanSound();
  }

  if (new_tag == true) {
    if (out == BUFF_LENGTH)
      out = 0;

    // Log data to SD card
    log2SD(buff[out]);
    // Send data only when a tag is scanned after the beam has been broken
    sendRequest(buff[out]);
    Serial.println(buff[out]);
    out++;
    new_tag = false;
  }
}