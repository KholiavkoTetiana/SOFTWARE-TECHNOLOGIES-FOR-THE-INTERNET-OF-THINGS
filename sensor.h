#pragma once
#include <Arduino.h>

// Драйвер датчика: тип обирається макросом SENSOR_TYPE у variant.h.
// Читання неблокуюче: довгі операції (перетворення DS18B20) розбиті на фази.
void        sensorBegin();
void        sensorUpdate(uint32_t now);   // викликати з loop() якомога частіше
bool        sensorValid();                // чи є свіже коректне значення
float       sensorValue();
const char* sensorName();                 // "temp" | "hum" | "lux"
const char* sensorUnit();                 // "C" | "%" | "lx"
const char* sensorPart();                 // назва моделі датчика
uint32_t    sensorErrors();               // кількість невдалих читань
