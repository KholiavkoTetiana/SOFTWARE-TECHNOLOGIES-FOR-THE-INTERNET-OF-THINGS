// ПТІР · Лабораторна робота №1 — базова прошивка ПЛК (шаблон).
// Світлодіод із ШІМ, кнопка з антибрязкотом і жестами, неблокуючий основний цикл, команди через Serial.
// Позначки TODO (Л1, завдання N) — ваші завдання (розділ 3 методички).
#include "board.h"
#include "variant.h"
#include "led.h"
#include "button.h"

enum class Mode : uint8_t { OFF, ON, BLINK, BREATH };
static const char* MODE_NAMES[] = { "OFF", "ON", "BLINK", "BREATH" };

static Mode mode = Mode::OFF;
static float brightness = 100;   // %
// TODO (Л1, завдання 4): додайте змінні, потрібні вашій схемі жестів (улюблена яскравість, блокування тощо).

// ---------- Патерн режиму BLINK: тривалості [увімк, вимк, увімк, вимк, …], мс ----------
// TODO (Л1, завдання 2): замініть приклад на патерн BLINK_PATTERN з вашого варіанта (таблиця патернів у методичці).
static const uint16_t PATTERN_STEPS[] = { 500, 500 };   // приклад: рівномірне блимання 1 Гц

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

// ---------- Жести → дії (схема з варіанта) ----------
static void onGesture(Gesture g) {
  if (g != Gesture::HOLD && g != Gesture::LONG_END) Serial.printf("GESTURE: %s\n", gestureName(g));

// TODO (Л1, завдання 4): дії для CLICK / DOUBLE / LONG за схемою GESTURE_SCHEME з вашого варіанта.
  // Тимчасово: клік вмикає і вимикає світлодіод.
  if (g == Gesture::CLICK) setMode(mode == Mode::OFF ? Mode::ON : Mode::OFF);
}

// ---------- Команди через Serial ----------
static void printStatus() {
  Serial.printf("STATUS board=\"%s\" target=%s mode=%s bright=%.0f scheme=%d pattern=%d "
                "pwm=%dHz/%dbit curve=%d duty=%lu/%lu uptime=%lums heap=%lu btnStack=%u\n",
                BOARD_NAME, TARGET_NAME, MODE_NAMES[(int)mode], brightness, GESTURE_SCHEME, BLINK_PATTERN,
                PWM_FREQ_HZ, PWM_RES_BITS, BRIGHT_CURVE, (unsigned long)ledDuty(), (unsigned long)ledMaxDuty(),
                (unsigned long)millis(), (unsigned long)ESP.getFreeHeap(), (unsigned)buttonStackFree());
}

static void handleCommand(char* line) {
  char* cmd = strtok(line, " ");
  char* arg = strtok(nullptr, " ");
  if (!cmd) return;
  if (!strcmp(cmd, "help")) {
    Serial.println("Команди: help | status | mode <off|on|blink|breath> | bright <0..100>");
  } else if (!strcmp(cmd, "status")) {
    printStatus();
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
    if (c == '\r') continue;
    if (c == '\n') { buf[len] = '\0'; handleCommand(buf); len = 0; }
    else if (len < sizeof(buf) - 1) buf[len++] = c;
  }
}

// ---------- setup / loop ----------
void setup() {
  Serial.begin(115200);
  delay(500);   // єдина затримка: даємо USB-Serial піднятися (лише під час старту)
  Serial.println();
  Serial.println("=== ПТІР · Лабораторна робота №1 ===");
  Serial.printf("Студент: %s | префікс: %s | варіант: %d\n", STUDENT_NAME, STUDENT_PREFIX, VARIANT_NO);
  if (!strcmp(STUDENT_PREFIX, "prizvyshche0101"))   // дані студента ще не заповнено
    Serial.println("УВАГА: задайте STUDENT_NAME і STUDENT_PREFIX у variant.h");
  Serial.printf("Плата: %s (%s) | чип: %s rev %d | ядро Arduino %d.%d.%d\n", BOARD_NAME, TARGET_NAME,
                ESP.getChipModel(), ESP.getChipRevision(),
                ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);

  ledBegin();
  buttonBegin();
  setMode(Mode::OFF);
  Serial.println("READY");
}

void loop() {
  // Чекаємо на жест не довше 10 мс: цикл реагує миттєво на події і при цьому віддає процесор іншим задачам.
  Gesture g;
  if (buttonGetGesture(g, pdMS_TO_TICKS(10))) onGesture(g);
  pollSerial();
  ledUpdate(millis());
}
