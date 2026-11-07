#pragma once
// Апаратна конфігурація: піни і рівні визначаються автоматично за профілем збірки (sketch.yaml).
// У коді вище цього рівня номери пінів і полярність не використовуються.
#include <Arduino.h>

#if defined(ARDUINO_LOLIN_S2_MINI)
  #define BOARD_NAME     "Wemos S2 Mini"
  #define LED_PIN        15
  #define LED_ACTIVE_LOW 0      // світлодіод світить при HIGH
  #define BUTTON_PIN     0      // кнопка «0» (BOOT)
  #if ARDUINO_USB_CDC_ON_BOOT
    #define TARGET_NAME  "плата"
  #else
    #define TARGET_NAME  "Wokwi" // у симуляторі Serial іде через UART0
  #endif
#elif defined(ARDUINO_NOLOGO_ESP32C3_SUPER_MINI)
  #define BOARD_NAME     "ESP32-C3 Super Mini"
  #define LED_PIN        8
  #define LED_ACTIVE_LOW 1      // на реальній платі світлодіод світить при LOW
  #define BUTTON_PIN     9      // кнопка BOOT
  #define TARGET_NAME    "плата"
#elif defined(ARDUINO_ESP32C3_DEV)
  #define BOARD_NAME     "ESP32-C3 Super Mini"
  #define LED_PIN        8
  #define LED_ACTIVE_LOW 0      // у моделі Wokwi світлодіод увімкнено інакше, ніж на платі
  #define BUTTON_PIN     9
  #define TARGET_NAME    "Wokwi"
#else
  #error "Невідома плата: збирайте з профілем із sketch.yaml (s2-wokwi, c3-wokwi, s2, c3)"
#endif

#define BUTTON_ACTIVE_LOW 1     // кнопка замикає пін на GND, увімкнена внутрішня підтяжка
