// ПТІР · Лабораторна робота №1 — базова прошивка ПЛК (шаблон).
// Світлодіод із ШІМ, кнопка з антибрязкотом і жестами, неблокуючий основний цикл, команди через Serial.
// Позначки TODO (Л1, завдання N) — ваші завдання (розділ 3 методички).
#include "board.h"
#include "variant.h"
#include "led.h"
#include "button.h"
#include "net.h"
#include "sensor.h"
#include "screen.h"

enum class Mode : uint8_t { OFF, ON, BLINK, BREATH };
static const char* MODE_NAMES[] = { "OFF", "ON", "BLINK", "BREATH" };

static Mode mode = Mode::OFF;
static float brightness = 100;   // %
// TODO (Л1, завдання 4): додайте змінні, потрібні вашій схемі жестів (улюблена яскравість, блокування тощо).

// ---------- Патерн режиму BLINK: тривалості [увімк, вимк, увімк, вимк, …], мс ----------
// TODO (Л1, завдання 2): замініть приклад на патерн BLINK_PATTERN з вашого варіанта (таблиця патернів у методичці).
static const uint16_t PATTERN_STEPS[] = { 200, 200, 200, 1000 };

static void selectPattern(const uint16_t*& steps, uint8_t& count) {
  steps = PATTERN_STEPS;
  count = sizeof(PATTERN_STEPS) / sizeof(PATTERN_STEPS[0]);
}

// ---------- Режими ----------
static void refreshLed() {
  switch (mode) {
    case Mode::OFF:    ledStatic(0); break;
    case Mode::ON:     ledStatic(brightness); break;
    case Mode::BLINK: {
      const uint16_t* steps; uint8_t count;
      selectPattern(steps, count);
      ledStartPattern(steps, count, brightness);
      break;
    }
    case Mode::BREATH: ledStartBreath(brightness, 3000); break;
  }
}

static void setMode(Mode m) {
  mode = m;
  refreshLed();
  Serial.printf("MODE: %s\n", MODE_NAMES[(int)mode]);
}

static void setBrightness(float b, bool report = true) {
  brightness = constrain(b, 0.0f, 100.0f);
  refreshLed();
  if (report) Serial.printf("BRIGHT: %.0f\n", brightness);
}

// ---------- Жести → дії (Варіант 20: Схема 2) ----------
static void onGesture(Gesture g) {
  if (g != Gesture::HOLD && g != Gesture::LONG_END)
    Serial.printf("GESTURE: %s\n", gestureName(g));

  switch (g) {
    case Gesture::CLICK:
      // Цикл режимів: OFF → ON → BLINK → BREATH → OFF
      switch (mode) {
        case Mode::OFF:    setMode(Mode::ON);     break;
        case Mode::ON:     setMode(Mode::BLINK);  break;
        case Mode::BLINK:  setMode(Mode::BREATH); break;
        case Mode::BREATH: setMode(Mode::OFF);    break;
      }
      break;

    case Gesture::DOUBLE:
      // Збільшення яскравості на 25% по колу: 25 → 50 → 75 → 100 → 25
      if (brightness >= 100) {
        setBrightness(25);
      } else {
        setBrightness(brightness + 25);
      }
      break;

    case Gesture::LONG:
      // Вимкнення з будь-якого стану
      setMode(Mode::OFF);
      break;

    case Gesture::HOLD:
    case Gesture::LONG_END:
      break;
  }
}

// ---------- Команди через Serial ----------
static void printStatus() {
  ApiResult a;
  netApi(a);
  char clk[9];
  netClock(clk, sizeof(clk));
  Serial.printf("STATUS board=\"%s\" target=%s mode=%s bright=%.0f scheme=%d pattern=%d "
                "pwm=%dHz/%dbit curve=%d duty=%lu/%lu uptime=%lums heap=%lu btnStack=%u "
                "net=%s ip=%s rssi=%d retries=%lu time=%s display=\"%s\" sensor=\"%s\" "
                "%s=%.2f%s errs=%lu api=\"%s\" value=%.4f%s ok=%lu fail=%lu last=%lums netStack=%u\n",
                BOARD_NAME, BUILD_TARGET, MODE_NAMES[(int)mode], brightness, GESTURE_SCHEME, BLINK_PATTERN,
                PWM_FREQ_HZ, PWM_RES_BITS, BRIGHT_CURVE, (unsigned long)ledDuty(), (unsigned long)ledMaxDuty(),
                (unsigned long)millis(), (unsigned long)ESP.getFreeHeap(), (unsigned)buttonStackFree(),
                netStateName(), netIp(), netRssi(), (unsigned long)netReconnects(), clk,
                screenPart(), sensorPart(),
                sensorName(), sensorValid() ? sensorValue() : NAN, sensorUnit(), (unsigned long)sensorErrors(),
                apiName(), a.valid ? a.value : NAN, apiUnit(),
                (unsigned long)a.okCount, (unsigned long)a.failCount, (unsigned long)a.durationMs,
                (unsigned)netStackFree());
}

static void printNet() {
  Serial.printf("NET state=%s ssid=%s ip=%s rssi=%d retries=%lu url=%s\n",
                netStateName(), WIFI_SSID, netIp(), netRssi(),
                (unsigned long)netReconnects(), apiUrl());
}

static void printTime() {
  struct tm t;
  char buf[48] = "not synchronized";
  if (netLocalTime(t)) strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z (day %j)", &t);
  Serial.printf("TIME: %s\n", buf);
}

static void printSensor() {
  if (sensorValid()) Serial.printf("SENSOR: %s=%.2f %s\n", sensorName(), sensorValue(), sensorUnit());
  else               Serial.printf("SENSOR: %s=- errors=%lu\n", sensorName(), (unsigned long)sensorErrors());
}

static void handleCommand(char* line) {
  char* cmd = strtok(line, " ");
  char* arg = strtok(nullptr, " ");
  if (!cmd) return;
  if (!strcmp(cmd, "help")) {
    Serial.println("Команди: help | status | mode <off|on|blink|breath> | bright <0..100>");
    Serial.println("         net | time | sensor | fetch | page <1..3|auto> | screen <on|off>");
  } else if (!strcmp(cmd, "status")) {
    printStatus();
  } else if (!strcmp(cmd, "net")) {
    printNet();
  } else if (!strcmp(cmd, "time")) {
    printTime();
  } else if (!strcmp(cmd, "sensor")) {
    printSensor();
  } else if (!strcmp(cmd, "fetch")) {
    netFetchNow();
    Serial.println("API: REQUESTED");
  } else if (!strcmp(cmd, "page") && arg) {
    screenSetPage(!strcasecmp(arg, "auto") ? -1 : (int8_t)atoi(arg));
    if (screenPageSetting() > 0) Serial.printf("PAGE: %d\n", screenPageSetting());
    else                         Serial.println("PAGE: AUTO");
  } else if (!strcmp(cmd, "screen") && arg) {
    screenSetEnabled(!strcasecmp(arg, "on"));
    Serial.printf("SCREEN: %s\n", screenEnabled() ? "ON" : "OFF");
  } else if (!strcmp(cmd, "mode") && arg) {
    for (int i = 0; i < 4; i++) {
      if (!strcasecmp(arg, MODE_NAMES[i])) { setMode((Mode)i); return; }
    }
    Serial.println("ERROR: невідомий режим");
  } else if (!strcmp(cmd, "bright") && arg) {
    setBrightness(atof(arg));
  } else {
    Serial.println("ERROR: невідома команда, введіть help");
  }
}

// Неблокуюче читання рядка: символи накопичуються, команда виконується після '\n'.
static void pollSerial() {
  static char buf[64];
  static uint8_t len = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (len > 0) { // Виконуємо команду, лише якщо буфер не порожній
        buf[len] = '\0'; 
        handleCommand(buf); 
        len = 0; 
      }
    }
    else if (len < sizeof(buf) - 1) {
      buf[len++] = c;
    }
  }
}

// ---------- Журнал подій мережі, датчика і запитів ----------
static void reportSensor() {
  static uint32_t lastLog = 0;
  static bool wasValid = false;
  uint32_t now = millis();
  const uint32_t SENSOR_LOG_MS = 10000;
  if (sensorValid() != wasValid || now - lastLog >= SENSOR_LOG_MS) {
    wasValid = sensorValid();
    lastLog = now;
    if (wasValid) Serial.printf("SENSOR: %s=%.2f %s\n", sensorName(), sensorValue(), sensorUnit());
    else          Serial.printf("SENSOR: %s=- errors=%lu\n", sensorName(), (unsigned long)sensorErrors());
  }
}

static void reportApi() {
  static uint32_t lastSeq = 0;
  ApiResult a;
  if (!netApi(a) || a.seq == lastSeq) return;
  lastSeq = a.seq;
  if (a.ok) Serial.printf("API: OK %s=%.4f %s http=%d t=%lums\n", apiName(), a.value, apiUnit(),
                          a.httpCode, (unsigned long)a.durationMs);
  else      Serial.printf("API: FAIL http=%d err=%s t=%lums\n", a.httpCode, a.error,
                          (unsigned long)a.durationMs);
}

static void fillUi(UiData& u) {
  ApiResult a;
  netApi(a);
  u.board = BOARD_SHORT;
  u.netState = netStateName();
  u.online = netOnline();
  u.rssi = netRssi();
  u.ip = netIp();
  u.ssid = WIFI_SSID;
  u.timeOk = netTimeValid();
  netClock(u.clock, sizeof(u.clock));
  u.sensorOk = sensorValid();
  u.sensorValue = sensorValue();
  u.sensorName = sensorName();
  u.sensorUnit = sensorUnit();
  u.apiOk = a.ok;
  u.apiValid = a.valid;
  u.apiValue = a.value;
  u.apiName = apiName();
  u.apiUnit = apiUnit();
  u.apiAgeS = a.valid ? (millis() - a.atMs) / 1000 : 0;
  u.apiError = a.error;
  u.mode = MODE_NAMES[(int)mode];
  u.bright = (int)brightness;
}

// ---------- setup / loop ----------
void setup() {
  Serial.begin(115200);
  delay(500);   // єдина затримка: даємо USB-Serial піднятися (лише під час старту)
  Serial.println();
  Serial.println("=== ПТІР · Лабораторна робота №2 ===");
  Serial.printf("Студент: %s | префікс: %s | варіант: %d\n", STUDENT_NAME, STUDENT_PREFIX, VARIANT_NO);
  if (!strcmp(STUDENT_PREFIX, "prizvyshche0101"))   // дані студента ще не заповнено
    Serial.println("УВАГА: задайте STUDENT_NAME і STUDENT_PREFIX у variant.h");
  Serial.printf("Плата: %s (%s) | чип: %s rev %d | ядро Arduino %d.%d.%d\n", BOARD_NAME, BUILD_TARGET,
                ESP.getChipModel(), ESP.getChipRevision(),
                ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
  Serial.printf("Варіант: дисплей %d, датчик %d, джерело %d (період %d с), помилка %d\n",
                DISPLAY_TYPE, SENSOR_TYPE, API_SOURCE, API_PERIOD_S, FAIL_POLICY);

  ledBegin();
  buttonBegin();

  bool screenOk = screenBegin();
  Serial.printf("DISPLAY: %s %s\n", screenPart(), screenOk ? "OK" : "FAIL");
  sensorBegin();
  Serial.printf("SENSOR: %s %s (%s, %s)\n", sensorPart(), "init", sensorName(), sensorUnit());
  netBegin();
  Serial.printf("API: %s <%s>\n", apiName(), apiUrl());

  setMode(Mode::OFF);
  Serial.println("READY");
}

void loop() {
  // Чекаємо на жест не довше 10 мс: цикл реагує миттєво на події і при цьому віддає процесор іншим задачам.
  Gesture g;
  if (buttonGetGesture(g, pdMS_TO_TICKS(10))) onGesture(g);
  pollSerial();

  uint32_t now = millis();
  ledUpdate(now);
  netUpdate(now);
  sensorUpdate(now);
  reportSensor();
  reportApi();

  UiData ui;
  fillUi(ui);
  screenUpdate(now, ui);
}
