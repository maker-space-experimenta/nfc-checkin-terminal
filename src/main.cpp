#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <FastLED.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include "config.h"


// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (14)
#define PN532_MOSI (12)
#define PN532_MISO (13)
#define PN532_SS   (15)

#define PN532_IRQ   (2)
#define PN532_RESET (3)

#define LED_PIN     0
#define NUM_LEDS    12
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define ANIM_SPEED  25   // animation speed in ms
#define ANIM_SCALE  64   // pulsing animation scaling

const char* ssid = CONFIG_WIFI_SSID;
const char* password = CONFIG_WIFI_PASSWORD;
const uint8_t fingerprint[20] = CONFIG_SSL_FINGERPRINT;
const String url_scan = CONFIG_BACKEND_URL;
const String url_alive = CONFIG_BACKEND_URL_ALIVE;
const String terminalId = CONFIG_TERMINAL_ID;
const int heartbeat_intervall = CONFIG_HEARTBEAT_INTERVAL;

ESP8266WiFiMulti WiFiMulti;
CRGB leds[NUM_LEDS];
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

void setColor(CRGB::HTMLColorCode color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }

  FastLED.show();
}

uint32_t lastAnimStep = 0;
uint8_t animCounter = 0;

void animationStep() {
  if(millis() - lastAnimStep >= ANIM_SPEED) {
    lastAnimStep = millis();

    uint8_t val = animCounter % ANIM_SCALE;                 // limit counter
    val = val < (ANIM_SCALE / 2) ? val : ANIM_SCALE - val;  // map to up/down counting
    val = map(val, 0, ANIM_SCALE / 2, 64, 255);             // map to full brightness

    for(uint8_t i = 0; i < NUM_LEDS; i++) {
      uint8_t hue = (animCounter + (255 / NUM_LEDS) * i) % 256; // hue rainbow
      leds[i] = CHSV(hue, 255, val);
    }

    FastLED.show();
    animCounter++;
  }
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
        
        
        setColor(CRGB::Blue);

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

                setColor(CRGB::Green);
                delay(500);
            } else {
              Serial.printf("[HTTPS] get failed, error: %s\n", https.errorToString(httpCode).c_str());
              setColor(CRGB::Red);
              delay(500);
            }
            setColor(CRGB::Black);
            delay(1000);
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

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );

  leds[0] = CRGB::Red;
  FastLED.show();

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
    setColor(CRGB::Red);
    delay(100);
    setColor(CRGB::Black);
    delay(100);
    setColor(CRGB::Red);
    delay(100);
    setColor(CRGB::Black);
    delay(100);

    disconnected = true;

    yield(); // feed WDT
  }

  if (disconnected) {
    Serial.print( "[WiFi] reconnected. IP: " );
    Serial.println( WiFi.localIP() );
    disconnected = false;
  }


  
  //setColor(CRGB::Black);

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 30);
  
  if (success) {
      String guid = guidToString(uid, uidLength);
      sendGuidToServer(guid);
  }

  //sendHeartbeat();

  animationStep();
}
