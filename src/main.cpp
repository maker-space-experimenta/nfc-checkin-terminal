#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <FastLED.h>


#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (14)
#define PN532_MOSI (12)
#define PN532_MISO (13)
#define PN532_SS   (15)

#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

#define LED_PIN     0
#define NUM_LEDS    1
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

const char* ssid = "<<wifi-ssid>>";
const char* password = "<<wifi-passwort>>";
WiFiClient client;

Adafruit_PN532 nfc(PN532_SS);

void setup(void) {
  Serial.begin(9600);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );

  leds[0] = CRGB::Red;
  FastLED.show();

  Wire.begin();
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
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
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  #ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
  #endif

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

  while (WiFi.status() != WL_CONNECTED) {
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
  }


  leds[0] = CRGB::Black;
  FastLED.show();

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) {
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    
    leds[0] = CRGB::Green;
    FastLED.show();
    delay(1000);
  }
}
