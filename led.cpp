#include "led.h"
#include "board.h"
#include "variant.h"
#include <math.h>

static const uint32_t MAX_DUTY = (1UL << PWM_RES_BITS) - 1;

enum class Drive : uint8_t { STATIC, PATTERN, BREATH };
static Drive drive = Drive::STATIC;
static float level = 0;                 // яскравість (для ефектів — максимальна), %
static const uint16_t* pattern = nullptr;
static uint8_t patternLen = 0, patternIdx = 0;
static uint32_t stepStart = 0;
static uint32_t breathPeriod = 3000, breathStart = 0;
static uint32_t duty = UINT32_MAX;      // останнє записане заповнення

// Перетворення «відсоток яскравості» (0..1) → «частка заповнення ШІМ» (0..1).
static float curve(float x) {
  x = constrain(x, 0.0f, 1.0f);
  // TODO (Л1, завдання 2): крива BRIGHT_CURVE з вашого варіанта (формули — у методичці, п. 1.1.7).
  return x;
}

static void writeDuty(uint32_t d) {
  if (d == duty) return;                // не смикаємо периферію без потреби
  duty = d;
  ledcWrite(LED_PIN, LED_ACTIVE_LOW ? MAX_DUTY - d : d);
}

static void writeLevel(float percent) {
  writeDuty((uint32_t)lroundf(curve(percent / 100.0f) * MAX_DUTY));
}

void ledBegin() {
  if (!ledcAttach(LED_PIN, PWM_FREQ_HZ, PWM_RES_BITS)) {
    Serial.println("ПОМИЛКА: ledcAttach — перевірте частоту і розрядність ШІМ");
  }
  writeLevel(0);
}

void ledStatic(float percent) {
  drive = Drive::STATIC;
  level = percent;
  writeLevel(percent);
}

void ledStartPattern(const uint16_t* steps, uint8_t count, float percent) {
  drive = Drive::PATTERN;
  pattern = steps;
  patternLen = count;
  patternIdx = 0;
  level = percent;
  stepStart = millis();
  writeLevel(percent);
}

void ledStartBreath(float percent, uint32_t periodMs) {
  drive = Drive::BREATH;
  level = percent;
  breathPeriod = periodMs;
  breathStart = millis();
}

void ledUpdate(uint32_t now) {
  switch (drive) {
    case Drive::STATIC:
      break;
    case Drive::PATTERN:
      // TODO (Л1, завдання 2): неблокуючий програвач патерну.
      //   pattern[patternIdx] — тривалість поточного кроку, мс; парні кроки — світить, непарні — пауза.
      //   Коли крок завершився (now - stepStart >= тривалість), перейти до наступного по колу.
      break;
    case Drive::BREATH:
      // TODO (Л1, завдання 2): «дихання» з періодом breathPeriod: яскравість 0 → level → 0 (трикутна хвиля).
      break;
  }
}

uint32_t ledDuty() { return duty; }
uint32_t ledMaxDuty() { return MAX_DUTY; }
