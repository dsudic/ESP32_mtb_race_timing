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

const char* ssid     = "Start";
const char* password = "timingXyZ";

const char* serverName = "http://pzi1.fesb.hr/~dsudic19/Timing/handle_post_from_start.php"; //http://pzi1.fesb.hr/~dsudic19/test/php/handle-form.php  http://pzi1.fesb.hr/~dsudic19/Timing-server/php/handle_post.php

String apiKeyValue = "******";

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;
byte prevUID[4]; // Init array that will store new NUID
String UID = "";

int buttonSync = 27; // button for sync on D1
int prevSyncState = HIGH;
int startSensor = 13; // light sensor on D2
int beamIndicator = 21; // LED indicator on D4
unsigned long startSyncTime;
unsigned long start = 0;

int prevReconnState = HIGH;
int reconn = 25;

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
unsigned long lastBuzz = 0; // prevMillis for buzzer indicator for beam

String SDlogData = "";
File SDfile;

int WiFiLED = 2;

void setup() {
  Serial.begin(115200);   // arduino 9600

  pinMode(buttonSync, INPUT_PULLUP);
  pinMode(reconn, INPUT_PULLUP);
  pinMode(startSensor, INPUT);
  pinMode(beamIndicator, OUTPUT);
  pinMode(WiFiLED, OUTPUT);

  ledcSetup(channel, freq, resolution); // buzzer
  ledcAttachPin(buzzer, channel); // buzzer

  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialization done!");

  //  //  uint8_t cardType = SD.cardType();
  //  //  if (cardType == CARD_NONE) {
  //  //    Serial.println("No SD card attached!");
  //  //    return;
  //  //  }

  SDfile = SD.open("/start.txt", FILE_APPEND);

  if (SDfile) {
    SDfile.println("-----------------------------------------------");
    SDfile.println("Chip_ID                |   Start[ms]");
    SDfile.println("");
    SDfile.close();
    Serial.println("Header line write done");
  }
  else
    Serial.println("ERROR opening file");


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

void sendRequest() {
  //Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");         // content-type header

    String httpRequestData = "api_key=" + apiKeyValue + "&start=" + (start - startSyncTime) + "&chip_id=" + UID;  // HTTP POST request data to be send

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
    UID += (buffer[i] < 0x16 ? "0" : "") + String(buffer[i], HEX) + (i != 3 ? " " : ""); // format UID string "XX XX XX XX"
  }
  UID.toUpperCase();
}

void getUID() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( !rfid.PICC_ReadCardSerial())
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
    /*new_tag = false;   --> BUG!! - ne treba ovo kod starta, onemogućava slanje zahtjeva
                                      serveru ukoliko se isti tag skenira vise
                                      puta zaredom, a izmedu toga ni jednom nije
                                      doslo do prekida zrake */
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

void log2SD() {
  SDlogData = UID + "   |   " + String(start - startSyncTime);
  SDfile = SD.open("/start.txt", FILE_APPEND);

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
  if (reconnState == LOW) {
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

  digitalWrite(WiFiLED, (WiFi.status() != WL_CONNECTED) ? HIGH : LOW);

  int buttonState = digitalRead(buttonSync);
  if (buttonState == LOW && prevSyncState == HIGH) {
    startSyncTime = millis();
    Serial.print("sync time on start gate = ");
    Serial.println(startSyncTime);
    
    // log sync times on sd card
    SDfile = SD.open("/sync_start.txt", FILE_APPEND);
    if (SDfile) {
      SDfile.println(startSyncTime);
      SDfile.close();
      Serial.println("Sync time written to SD card");
    }
    else
      Serial.println("ERROR opening file");
  }
  prevSyncState = buttonState;

  int sensorState_start = digitalRead(startSensor);
  digitalWrite(beamIndicator, !sensorState_start);  // if beam is broken, turn LED on
  //digitalWrite(12, !sensorState_start); // testing w/ stopwatch

  getUID();

  if (!buzz_flag) {
    RFIDScanSound();
  }


  if (sensorState_start == LOW && new_tag == true) { // if the beam is broken and a NEW tag already scanned
    start = millis();
    Serial.print("start time: ");
    Serial.println(start);
    Serial.print("sync time: ");
    Serial.println(startSyncTime);
    Serial.print("--> Real start time (start - sync): ");
    Serial.println(start - startSyncTime);
    ledcWrite(channel, 3000);
    lastBuzz = millis();
    log2SD();  // log data to SD card
    sendRequest();
    //flag = true;
    //delay(1000);
    new_tag = false;
  }
  if (millis() - lastBuzz > 400 && buzz_flag)
    ledcWrite(channel, 0);    // turn the buzzer for 400ms if the beam has been broken
}