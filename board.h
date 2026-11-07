#pragma once
// Апаратна конфігурація: піни і рівні визначаються автоматично за профілем збірки (sketch.yaml).
// У коді вище цього рівня номери пінів і полярність не використовуються.
#include <Arduino.h>

// Назва BUILD_TARGET, а не TARGET_NAME: останнє вже зайняте бібліотекою DHT
// (див. DHT.h) — однакові імена макросів у різних файлах конфліктують.
#if defined(ARDUINO_LOLIN_S2_MINI)
  #define BOARD_NAME     "Wemos S2 Mini"
  #define BOARD_SHORT    "S2"
  #define LED_PIN        15
  #define LED_ACTIVE_LOW 0      // світлодіод світить при HIGH
  #define BUTTON_PIN     0      // кнопка «0» (BOOT)
  #if ARDUINO_USB_CDC_ON_BOOT
    #define BUILD_TARGET  "плата"
  #else
    #define BUILD_TARGET  "Wokwi" // у симуляторі Serial іде через UART0
  #endif
#elif defined(ARDUINO_NOLOGO_ESP32C3_SUPER_MINI)
  #define BOARD_NAME     "ESP32-C3 Super Mini"
  #define BOARD_SHORT    "C3"
  #define LED_PIN        8
  #define LED_ACTIVE_LOW 1      // на реальній платі світлодіод світить при LOW
  #define BUTTON_PIN     9      // кнопка BOOT
  #define BUILD_TARGET    "плата"
#elif defined(ARDUINO_ESP32C3_DEV)
  #define BOARD_NAME     "ESP32-C3 Super Mini"
  #define BOARD_SHORT    "C3"
  #define LED_PIN        8
  #define LED_ACTIVE_LOW 0      // у моделі Wokwi світлодіод увімкнено інакше, ніж на платі
  #define BUTTON_PIN     9
  #define BUILD_TARGET    "Wokwi"
#else
  #error "Невідома плата: збирайте з профілем із sketch.yaml (s2-wokwi, c3-wokwi, s2, c3)"
#endif

#define BUTTON_ACTIVE_LOW 1     // кнопка замикає пін на GND, увімкнена внутрішня підтяжка

// ===== Периферія лабораторної роботи №2 (піни однакові на обох платах) =====
#define I2C_SDA_PIN       4    // шина I2C: дисплеї SSD1306 (0x3C) і LCD 1602 (0x27)
#define I2C_SCL_PIN       5
#define SENSOR_ANALOG_PIN 3    // NTC-термістор, фоторезистор (канал ADC1)
#define SENSOR_DATA_PIN   7    // DHT22, DS18B20
#define SEG_CLK_PIN       1    // TM1637
#define SEG_DIO_PIN       2
