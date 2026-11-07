#pragma once
#include <Arduino.h>

enum class Gesture : uint8_t {
  CLICK,      // одне коротке натискання
  DOUBLE,     // два натискання у вікні DOUBLE_CLICK_MS
  LONG,       // утримання довше LONG_PRESS_MS (подія в момент досягнення порога)
  HOLD,       // повторюється кожні HOLD_REPEAT_MS, поки кнопку утримують після LONG
  LONG_END    // кнопку відпущено після LONG
};

void buttonBegin();                                  // створює задачу FreeRTOS і чергу подій
bool buttonGetGesture(Gesture& g, TickType_t wait);  // чекає подію не довше wait тиків
const char* gestureName(Gesture g);
UBaseType_t buttonStackFree();                       // мінімальний залишок стеку задачі за весь час роботи, байт (ESP-IDF рахує в байтах)
