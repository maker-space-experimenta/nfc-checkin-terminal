#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <FastLED.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include <config.h>


// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (14)
#define PN532_MOSI (12)
#define PN532_MISO (13)
#define PN532_SS   (15)

#define PN532_IRQ   (2)
#define PN532_RESET (3)

#define LED_PIN     0
#define NUM_LEDS    1
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB

const char* ssid = CONFIG_WIFI_SSID;
const char* password = CONFIG_WIFI_PASSWORD;
const uint8_t fingerprint[20] = CONFIG_SSL_FINGERPRINT;
const String server = CONFIG_BACKEND_URL;

ESP8266WiFiMulti WiFiMulti;
CRGB leds[NUM_LEDS];
Adafruit_PN532 nfc(PN532_SS);
bool disconnected = true;

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

bool sendGuidToServer(String guid) {
    String url = server + guid;
    Serial.println(url);
    bool success = false;

    if ((WiFiMulti.run() == WL_CONNECTED)) {

        std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
        client->setFingerprint(fingerprint);
        
        HTTPClient https;
        if (https.begin(*client, url)) {
            int httpCode = https.GET();
            Serial.print("HTTP-Code ");
            Serial.println(httpCode);

            if ( httpCode > 0 && httpCode == HTTP_CODE_OK ) {
                String payload = https.getString();
                Serial.println(payload);
                success = true;
            } else {
              Serial.printf("[HTTPS] get failed, error: %s\n", https.errorToString(httpCode).c_str());
            }
        }
    
        https.end(); 
    }

    return success;
}

void setup(void) {
  Serial.begin(9600);

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
    leds[0] = CRGB::Black;
    FastLED.show();
    delay(100);
    leds[0] = CRGB::Black;
    FastLED.show();
    delay(100);
    leds[0] = CRGB::Black;
    FastLED.show();
    delay(100);
    leds[0] = CRGB::Black;
    FastLED.show();
    delay(100);

    disconnected = true;
  }

  if (disconnected) {
    Serial.print( "[WiFi] reconnected. IP: " );
    Serial.println( WiFi.localIP() );
    disconnected = false;
  }


  leds[0] = CRGB::Black;
  FastLED.show();

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) {
      String guid = guidToString(uid, uidLength);
      if (sendGuidToServer(guid)) {
          leds[0] = CRGB::Green;
          FastLED.show();
          delay(1000);
      } else {
          leds[0] = CRGB::Red;
          FastLED.show();
          delay(1000);
      }
    
    
  }
}
