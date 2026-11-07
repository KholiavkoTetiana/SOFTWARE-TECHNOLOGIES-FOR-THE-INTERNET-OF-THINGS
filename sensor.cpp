#include "sensor.h"
#include "board.h"
#include "variant.h"
#include <math.h>

#if SENSOR_TYPE == 1 || SENSOR_TYPE == 2
  #include <DHT.h>
  static DHT dht(SENSOR_DATA_PIN, DHT22);
#elif SENSOR_TYPE == 5
  #include <OneWire.h>
  #include <DallasTemperature.h>
  static OneWire oneWire(SENSOR_DATA_PIN);
  static DallasTemperature ds(&oneWire);
  static const uint32_t DS_CONVERT_MS = 800;   // 12 біт: 750 мс за паспортом, беремо із запасом
  static bool converting = false;
  static uint32_t convertStart = 0;
#endif

// Параметри аналогових датчиків (перевірені прогонами в Wokwi, див. п. 2.1.5 методички).
static const float ADC_VREF   = 3.3f;     // у симуляторі АЦП лінійний від 0 до 3,3 В
static const float ADC_MAX    = 4095.0f;  // 12 біт
static const float R_SERIES   = 10000.0f; // резистор дільника в модулі датчика, Ом
static const float NTC_R25    = 10000.0f; // опір термістора при 25 °C, Ом
static const float NTC_BETA   = 3950.0f;  // коефіцієнт B термістора, К
static const float LDR_R10    = 50000.0f; // опір фоторезистора при 10 лк, Ом
static const float LDR_GAMMA  = 0.7f;     // показник степеня характеристики фоторезистора

static float value = NAN;
static bool valid = false;
static uint32_t errors = 0;
static uint32_t lastRead = 0;

#if SENSOR_TYPE == 3 || SENSOR_TYPE == 4
// Напруга на аналоговому вході, В. У симуляторі АЦП ідеально лінійний,
// тому переводимо код напряму. На реальній платі треба analogReadMilliVolts().
static float analogVolts() {
  return analogRead(SENSOR_ANALOG_PIN) / ADC_MAX * ADC_VREF;
}
// Опір нижнього плеча дільника (самого датчика), Ом.
static float dividerResistance() {
  float u = analogVolts();
  if (u <= 0.01f || u >= ADC_VREF - 0.01f) return NAN;   // обрив або коротке замикання
  // TODO (Л2, завдання 4): виразіть опір датчика через виміряну напругу
  //   (формула дільника — п. 2.1.5 методички).
  return NAN;
}
#endif

void sensorBegin() {
#if SENSOR_TYPE == 1 || SENSOR_TYPE == 2
  dht.begin();
#elif SENSOR_TYPE == 3 || SENSOR_TYPE == 4
  analogReadResolution(12);
  analogSetPinAttenuation(SENSOR_ANALOG_PIN, ADC_11db);   // повна шкала ~3,3 В
#elif SENSOR_TYPE == 5
  ds.begin();
  ds.setResolution(12);
  ds.setWaitForConversion(false);   // не блокувати програму на 750 мс перетворення
#endif
  lastRead = millis() - SENSOR_PERIOD_MS;
}

void sensorUpdate(uint32_t now) {
#if SENSOR_TYPE == 5
  // Дві фази: запустити перетворення — і забрати результат через DS_CONVERT_MS.
  if (!converting) {
    if (now - lastRead < SENSOR_PERIOD_MS) return;
    ds.requestTemperatures();
    convertStart = now;
    converting = true;
    return;
  }
  if (now - convertStart < DS_CONVERT_MS) return;
  converting = false;
  lastRead = now;
  // TODO (Л2, завдання 4): заберіть результат ds.getTempCByIndex(0) і перевірте його
  //   на DEVICE_DISCONNECTED_C та NaN; у разі помилки — valid = false і errors++.
  float t = ds.getTempCByIndex(0);
  value = t;
  valid = true;
#else
  if (now - lastRead < SENSOR_PERIOD_MS) return;
  lastRead = now;
  float v;
  // TODO (Л2, завдання 4): читання датчика свого варіанта.
  #if SENSOR_TYPE == 1
    v = 20.0f;   // замініть: температура з DHT22 (одне звертання до бібліотеки)
  #elif SENSOR_TYPE == 2
    v = 50.0f;   // замініть: відносна вологість з DHT22
  #elif SENSOR_TYPE == 3
    // Рівняння з коефіцієнтом B (п. 2.1.5): 1/T = 1/T25 + ln(R/R25)/B, T — у кельвінах.
    float r = dividerResistance();
    v = isnan(r) ? NAN : 20.0f;   // замініть на температуру в градусах Цельсія
  #elif SENSOR_TYPE == 4
    // Характеристика фоторезистора (п. 2.1.5): R = R10·(E/10)^(−gamma).
    float r = dividerResistance();
    v = isnan(r) ? NAN : 100.0f;  // замініть на освітленість у люксах
  #else
    #error "SENSOR_TYPE має бути 1..5"
  #endif
  if (isnan(v)) { valid = false; errors++; return; }
  value = v;
  valid = true;
#endif
}

bool  sensorValid() { return valid; }
float sensorValue() { return value; }
uint32_t sensorErrors() { return errors; }

const char* sensorName() {
#if SENSOR_TYPE == 2
  return "hum";
#elif SENSOR_TYPE == 4
  return "light";
#else
  return "temp";
#endif
}

const char* sensorUnit() {
#if SENSOR_TYPE == 2
  return "%";
#elif SENSOR_TYPE == 4
  return "lx";
#else
  return "C";
#endif
}

const char* sensorPart() {
#if SENSOR_TYPE == 1 || SENSOR_TYPE == 2
  return "DHT22";
#elif SENSOR_TYPE == 3
  return "NTC 10k B=3950";
#elif SENSOR_TYPE == 4
  return "photoresistor";
#else
  return "DS18B20";
#endif
}
