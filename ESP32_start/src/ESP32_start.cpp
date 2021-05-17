#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include<SD.h>

#define SS_PIN 5 // Chip Select pin for RFID reader
#define RST_PIN 22 // RFID reader RST pin
#define SD_CS 15  // Chip Select pin for SD card reader
#define BUFF_LENGTH 5  // size of an array for temporary storing finish times

const char* ssid     = "Finish";
const char* password = "timingXyZ";

const char* serverName = "http://pzi1.fesb.hr/~dsudic19/Timing/handle_post_from_finish.php";
String apiKeyValue = "*******";

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;
byte prevUID[4]; // Init array that will store new NUID
String UID = "";

int buttonSync = 27; // button for sync on D1
int prevSyncState = HIGH;
int finishSensor = 13; // light sensor on D2
int beamIndicator = 21; // LED indicator on D4
unsigned long finishSyncTime;
unsigned long finish = 0;
unsigned long interval = 1500;  // interval in which sensor is not sensing after the beam is broken
int previousState = 0;

int prevReconnState = HIGH;
int reconn = 25;

unsigned long buff[BUFF_LENGTH] = {0};  // array for temporary storing the finish times to avoid needing the rider to scan his RFID quickly before new a rider crosses the finish line
int in = 0;   // index where new value will be stored in an array
int out = 0;    // finish time from this index will be sent next to the server/SD card
const int resetIndexes = 32; // button for reseting buffer indexes(in, out) on pin 32
int prevStateRI = HIGH;
int currentStateRI;
unsigned long pressedTimeRI = 0;
bool pressingRI = false;
bool longDetectedRI = false;

const int falseFinish = 33;   // button for overwriting last value in buffer in case someone crossed finish accidentally
int prevStateFF = HIGH;
int currentStateFF;
unsigned long pressedTimeFF = 0;
bool pressingFF = false;
bool longDetectedFF = false;

boolean new_tag = false;

// buzzer
int buzzer = 26;
int freq = 3000;
int channel = 0;
int resolution = 8;
unsigned long previousMillis = 0; // last time buzzer changed state
unsigned long buzzerOn = 100;
unsigned long buzzerOff = 10;
boolean buzz_flag = true;         // permission to call RFIDScanSound function (!buzz_flag)
int counter = 0;                  // in which phase is beeping (On(100ms)-Off(10ms)-On(100ms)-Off)
unsigned long lastBuzz = 0; // prevMillis for buzzer indicator for beam and buttons

unsigned long prevMillis = 0;
unsigned long scanInterval = 3000;

String SDlogData = "";
File SDfile;

void setup() {
  Serial.begin(115200); // arduino 9600

  pinMode(buttonSync, INPUT_PULLUP);
  pinMode(reconn, INPUT_PULLUP);
  pinMode(finishSensor, INPUT);
  pinMode(beamIndicator, OUTPUT);
  pinMode(resetIndexes, INPUT);
  pinMode(falseFinish, INPUT);
  //pinMode(14, OUTPUT); // testing w/ stopwatch
  //digitalWrite(14, LOW); // testing w/ stopwatch

  ledcSetup(channel, freq, resolution); // buzzer
  ledcAttachPin(buzzer, channel); // buzzer

  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    return; // ili while(1);
  }
  Serial.println("SD card initialization done!");

  //  uint8_t cardType = SD.cardType();
  //  if (cardType == CARD_NONE) {
  //    Serial.println("No SD card attached!");
  //    return;
  //  }

  SDfile = SD.open("/finish.txt", FILE_WRITE);

  if (SDfile) {
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

//void ReconnectWiFi(WiFiEvent_t event, WiFiEventInfo_t info){
//  Serial.print("WiFi lost connection. Reason: ");
//  Serial.println(info.disconnected.reason);
//  Serial.println("Trying to Reconnect");
//  WiFi.begin(ssid, password);
//}

void sendRequest(unsigned int finishTotal) {
  //Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");         // content-type header

    String httpRequestData = "api_key=" + apiKeyValue + "&finish=" + finishTotal + "&chip_id=" + UID;  // HTTP POST request data to be send

    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);
    Serial.println("---------------------------------");

    int httpResponseCode = http.POST(httpRequestData);        // Send HTTP POST request

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }

    http.end();    // Free resources
  }

  else {
    Serial.println("WiFi Disconnected");
  }
}


void stringifyUID(byte *buffer, byte bufferSize) {
  UID = "";
  for (byte i = 0; i < bufferSize; i++) {
    UID += (buffer[i] < 0x16 ? "0" : "") + String(buffer[i], HEX) + (i != 3 ? " " : "");  // format UID string "XX XX XX XX"
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

  if (rfid.uid.uidByte[0] != prevUID[0] ||
      rfid.uid.uidByte[1] != prevUID[1] ||
      rfid.uid.uidByte[2] != prevUID[2] ||
      rfid.uid.uidByte[3] != prevUID[3]) {    // Check if it is the same tag as previously scanned
    Serial.println(F("A new card has been detected."));

    new_tag = true;

    ledcWriteTone(channel, 3000); // turn a buzzer On
    previousMillis = millis();  // mark when a buzzer started
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
    //onn = false;
    previousMillis = millis();
    counter++;
  }

  if ((millis() - previousMillis) >= buzzerOff && counter == 1) {
    ledcWriteTone(channel, 3000);
    //onn = true;
    previousMillis = millis();
    counter++;
  }

  if ((millis() - previousMillis) > buzzerOn && counter == 2) {
    ledcWrite(channel, 0);
    //onn = false;
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


void loop() {

  if ((WiFi.status() != WL_CONNECTED) && (millis() - prevMillis > scanInterval)) {

    WiFi.begin(ssid, password);
    Serial.println("Connecting...");
    //    while (WiFi.status() != WL_CONNECTED) {
    //    delay(500);
    //    Serial.print(".");
    //    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.print("Reconnected to WiFi network with IP Address: ");
      Serial.println(WiFi.localIP());
    }

    prevMillis = millis();
  }

  // to reset IN and OUT indexes for buff array, press and hold button for 1.5s
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



  // in case of a false beam break, press button for 1.5s to overwrite last value in a buff array
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
      ledcWrite(channel, 3000);  // if the button is pressed for 1.5s turn the buzzer on
      lastBuzz = millis();
    }
  prevStateFF = currentStateFF;

  if (millis() - lastBuzz >= 400 && longDetectedFF)  // turn the buzzer off after 400ms
    ledcWrite(channel, 0);

  int buttonState = digitalRead(buttonSync);
  if (buttonState == LOW && prevSyncState == HIGH) {
    finishSyncTime = millis();
    Serial.print("sync time on finish gate = ");
    Serial.println(finishSyncTime);
    //delay(1000);
  }
  prevSyncState = buttonState;

  int sensorState_finish = digitalRead(finishSensor);
  digitalWrite(beamIndicator, !sensorState_finish);  // if beam is broken, turn LED on
  //digitalWrite(14, !sensorState_finish); // testing w/ stopwatch

  if (sensorState_finish == LOW && sensorState_finish != previousState && (millis() - finish) > interval) {  // to avoid multiple beam interruptions as the bicycle passes trough finish line
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
    buff[in++] = finish - finishSyncTime;  // temporary store the finish time in an array

    for (int i = 0; i < BUFF_LENGTH; i++)  // for watching changes in the buffer
      Serial.println(buff[i]);
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

    log2SD(buff[out]); // log data to SD card
    sendRequest(buff[out]);    // send data only when a tag is scanned after the beam has been broken
    Serial.println(buff[out]);
    out++;
    new_tag = false;
  }
}