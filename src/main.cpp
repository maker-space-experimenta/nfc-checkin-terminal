#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include "config.h"
#include "led.h"

// TODO:
// - Check-in / check-out / not found animation


// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (14)
#define PN532_MOSI (12)
#define PN532_MISO (13)
#define PN532_SS   (15)

#define PN532_IRQ   (2)
#define PN532_RESET (3)

const char* ssid = CONFIG_WIFI_SSID;
const char* password = CONFIG_WIFI_PASSWORD;
const uint8_t fingerprint[20] = CONFIG_SSL_FINGERPRINT;
const String url_scan = CONFIG_BACKEND_URL;
const String url_alive = CONFIG_BACKEND_URL_ALIVE;
const String terminalId = CONFIG_TERMINAL_ID;
const int heartbeat_intervall = CONFIG_HEARTBEAT_INTERVAL;

ESP8266WiFiMulti WiFiMulti;
Adafruit_PN532 nfc(PN532_SS);
bool disconnected = true;
ulong lastHeartbeat = 0;

String guidToString(uint8_t uid[], uint8_t uidLength) {

    String guid = "";

    for (int i = 0; i < uidLength; i++) {
        String partial = String(uid[i], HEX);
        
        if (partial.length() < 2)
            partial = "0" + partial;

        guid += partial;
        if (i < uidLength-1) {
            guid += ":";
        }
    }

    return guid;
}

int sendHttpsGet(String url) {
    Serial.println(url);
    int httpCode = 0;
    
    if ((WiFiMulti.run() == WL_CONNECTED)) {

        std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
        client->setFingerprint(fingerprint);
        
        HTTPClient https;
        if (https.begin(*client, url)) {
            httpCode = https.GET();
        }
    
        https.end(); 
    }

    return httpCode;
}

bool sendGuidToServer(String guid) {
    String url = url_scan + guid;
    Serial.println(url);
    bool success = false;

    if ((WiFiMulti.run() == WL_CONNECTED)) {

        std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
        client->setFingerprint(fingerprint);
        
        
        setAnimation(ANIM_CARD_PROCESSING);
        animationLoop(true);

        HTTPClient https;
        https.setTimeout(5000);
        if (https.begin(*client, url)) {
            int httpCode = https.GET();
            Serial.print("HTTP-Code ");
            Serial.println(httpCode);

            if ( httpCode > 0 && httpCode == HTTP_CODE_OK ) {
                String payload = https.getString();
                Serial.println(payload);
                success = true;

                if(payload.indexOf("added") > -1) {
                  setAnimation(ANIM_CHECK_IN);
                }
                else if(payload.indexOf("removed") > -1) {
                  setAnimation(ANIM_CHECK_OUT);
                }
                else if(payload.indexOf("unknown") > -1) {
                  setAnimation(ANIM_UNKNOWN_CARD);
                }
            } else {
              Serial.printf("[HTTPS] get failed, error: %s\n", https.errorToString(httpCode).c_str());
              setAnimation(ANIM_ERROR, 800);
            }
        }
    
        https.end(); 
    }

    return success;
}

bool sendHeartbeat() {
    String url = url_alive + terminalId;
    Serial.println(url);
    bool success = false;
    int httpCode = sendHttpsGet(url);

    if ( httpCode == HTTP_CODE_OK ) {
        success = true;
    } else {
      Serial.printf("[HTTPS] get failed, error: %i\n", httpCode);
    }

    return success;
}

void setup(void) {
  Serial.begin(115200);

  initLeds();
  setAnimation(ANIM_CONNECTING);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);

  
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  
  Serial.println("Waiting for an ISO14443A Card ...");
}

void loop(void) {

  while (WiFiMulti.run() != WL_CONNECTED) {
    if(disconnected == false) { // only run once
      setAnimation(ANIM_CONNECTING);
    }
    disconnected = true;

    animationLoop();

    yield(); // feed WDT
  }

  if (disconnected) {
    disconnected = false;
    Serial.print( "[WiFi] reconnected. IP: " );
    Serial.println( WiFi.localIP() );
    setAnimation(ANIM_IDLE);
  }


  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 30);
  
  if (success) {
      String guid = guidToString(uid, uidLength);
      sendGuidToServer(guid);
  }

  //sendHeartbeat();

  animationLoop();
}
