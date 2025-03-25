#pragma once
#include "stdlib.h"
// class Hardware
// {
//     static bool isFlashing;

// public:
// static void init();
void ledOn();
void ledOff();
void ledPut(uint8_t state);
void ledToggle();
//     static void ledFlashing(uint32_t interval);
// };