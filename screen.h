#pragma once
#include <Arduino.h>

// Дані, які показує дисплей. Формує loop(), драйвер дисплея лише малює:
// так модуль екрана не залежить від решти проєкту.
struct UiData {
  const char* board;       // коротка назва плати для заголовка
  const char* netState;
  bool        online;
  int         rssi;
  const char* ip;
  const char* ssid;
  char        clock[9];      // "ГГ:ХХ:СС" або "--:--:--"
  bool        timeOk;

  bool        sensorOk;
  float       sensorValue;
  const char* sensorName;
  const char* sensorUnit;

  bool        apiOk;         // остання спроба успішна
  bool        apiValid;      // є хоч якесь значення
  float       apiValue;
  const char* apiName;
  const char* apiUnit;
  uint32_t    apiAgeS;       // скільки секунд тому отримано значення
  const char* apiError;

  const char* mode;
  int         bright;
};

// Драйвер дисплея: тип обирається макросом DISPLAY_TYPE у variant.h.
bool        screenBegin();                       // false — дисплей не відгукнувся
bool        screenReady();
const char* screenPart();                        // назва моделі дисплея
uint8_t     screenPages();
void        screenSetPage(int8_t page);          // 1..3, або -1 — автоматичне гортання
int8_t      screenPageSetting();                 // що задано: 1..3 або -1
uint8_t     screenCurrentPage();                 // що зараз на екрані
void        screenSetEnabled(bool on);
bool        screenEnabled();
void        screenUpdate(uint32_t now, const UiData& d);   // гортання + перемальовка
