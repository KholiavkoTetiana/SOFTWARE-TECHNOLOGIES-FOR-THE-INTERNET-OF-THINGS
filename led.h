#pragma once
#include <Arduino.h>

// Драйвер світлодіода: ШІМ (LEDC), крива яскравості, полярність, неблокуючі ефекти.
void ledBegin();
void ledStatic(float percent);                                     // постійна яскравість 0..100 %
void ledStartPattern(const uint16_t* steps, uint8_t count, float percent); // [увімк, вимк, увімк, …], мс
void ledStartBreath(float percent, uint32_t periodMs);             // плавне «дихання»
void ledUpdate(uint32_t nowMs);                                    // викликати якомога частіше з loop()
uint32_t ledDuty();                                                // поточне заповнення ШІМ (без інверсії)
uint32_t ledMaxDuty();
