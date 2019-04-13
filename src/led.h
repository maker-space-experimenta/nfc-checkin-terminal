#pragma once

#include <Arduino.h>
#include <FastLED.h>

#define LED_PIN     0
#define NUM_LEDS    12
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define ANIM_SPEED  25 // animation speed in ms
#define ANIM_SCALE  64 // pulsing animation scaling

extern CRGB leds[NUM_LEDS];

extern uint32_t lastAnimStep;
extern uint8_t animCounter;


void initLeds();
void animationLoop();

void setColor(CRGB::HTMLColorCode color);
