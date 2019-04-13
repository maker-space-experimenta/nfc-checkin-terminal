#include "led.h"

CRGB leds[NUM_LEDS];
uint32_t lastAnimStep = 0;
uint8_t animCounter = 0;

void initLeds() {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);

    leds[0] = CRGB::Red;
    FastLED.show();

    lastAnimStep = 0;
    animCounter = 0;
}

void setColor(CRGB::HTMLColorCode color) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = color;
    }

    FastLED.show();
}

void animationLoop() {
    if (millis() - lastAnimStep >= ANIM_SPEED) {
        lastAnimStep = millis();

        uint8_t val = animCounter % ANIM_SCALE;                // limit counter
        val = val < (ANIM_SCALE / 2) ? val : ANIM_SCALE - val; // map to up/down counting
        val = map(val, 0, ANIM_SCALE / 2, 64, 255);            // map to full brightness

        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            uint8_t hue = (animCounter + (255 / NUM_LEDS) * i) % 256; // hue rainbow
            leds[i] = CHSV(hue, 255, val);
        }

        FastLED.show();
        animCounter++;
    }
}