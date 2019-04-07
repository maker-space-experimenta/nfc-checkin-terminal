# NFC CheckIn Terminal

This repository contains code for a WEMOS D1 based terminal for tracking users of our maker space.

A PN532 is used to capture the GUID of a Mifare card. This ID is send to a webserver for further processing.

A WS2811 LED is used to visualize the reading success.


## Pinout WEMOS

| pin | function
| --- | ---
| D3  | WS2812 LED
| D5  | SCK
| D6  | MISO
| D7  | SSEL
| D8  | MOSI
