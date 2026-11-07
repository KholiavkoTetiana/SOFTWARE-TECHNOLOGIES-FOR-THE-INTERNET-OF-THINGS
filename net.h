#pragma once
#include <Arduino.h>
#include <time.h>

// Мережа: неблокуюче підключення до Wi-Fi, синхронізація часу (NTP)
// і запит до джерела даних в окремій задачі FreeRTOS.

enum class NetState : uint8_t { IDLE, CONNECTING, ONLINE, WAITING };

struct ApiResult {
  uint32_t seq;        // номер результату: зростає після кожної спроби
  bool ok;             // остання спроба успішна
  bool valid;          // хоч одна спроба була успішною (у value є число)
  float value;         // розібране значення
  uint32_t atMs;       // millis() останнього успіху
  int httpCode;        // код відповіді (< 0 — помилка HTTPClient)
  uint32_t durationMs; // тривалість останньої спроби
  uint32_t okCount;
  uint32_t failCount;
  char error[40];      // текст помилки останньої невдалої спроби
};

void        netBegin();
void        netUpdate(uint32_t now);      // викликати з loop() якомога частіше
bool        netOnline();
NetState    netState();
const char* netStateName();
int         netRssi();
const char* netIp();
uint32_t    netReconnects();

bool        netTimeValid();
bool        netLocalTime(struct tm& out);
void        netClock(char* buf, size_t len);   // "ГГ:ХХ:СС" або "--:--:--"

void        netFetchNow();                     // позачерговий запит до джерела даних
bool        netApi(ApiResult& out);            // знімок результату (під м'ютексом)
const char* apiName();
const char* apiUnit();
const char* apiUrl();
UBaseType_t netStackFree();                    // залишок стеку задачі запитів, байт
