#include "button.h"
#include "board.h"
#include "variant.h"

static const uint32_t POLL_MS = 5;          // період опитування кнопки
static const uint32_t HOLD_REPEAT_MS = 50;  // період подій HOLD

static QueueHandle_t queue = nullptr;
static TaskHandle_t task = nullptr;

static inline bool pressed() {
  return digitalRead(BUTTON_PIN) == (BUTTON_ACTIVE_LOW ? LOW : HIGH);
}

static void emit(Gesture g) {
  xQueueSend(queue, &g, 0);   // не блокуємо задачу кнопки; якщо черга повна — подія губиться
}

// Задача кнопки: антибрязкіт + автомат станів розпізнавання жестів.
static void buttonTask(void*) {
  enum State : uint8_t { IDLE, DOWN1, WAIT2, DOWN2, HELD } state = IDLE;
  bool raw = pressed(), stable = raw;
  uint32_t rawChangedAt = millis(), t = 0, lastRepeat = 0;

  for (;;) {
    uint32_t now = millis();

    // 1. Антибрязкіт: новий стан приймається, лише якщо він стабільний DEBOUNCE_MS.
    bool r = pressed();
    if (r != raw) { raw = r; rawChangedAt = now; }
    bool down = false, up = false;
    if (raw != stable && now - rawChangedAt >= DEBOUNCE_MS) {
      stable = raw;
      if (stable) down = true; else up = true;
    }

    // 2. TODO (Л1, завдання 3): автомат станів жестів CLICK / DOUBLE / LONG (+ HOLD, LONG_END для схеми 6).
    //    Діаграма станів — у методичці, п. 1.1.5. Змінні state, t, lastRepeat підготовлено для вас.
    //    Тимчасова заглушка: кожне натискання одразу вважається кліком.
    
    // 2. Розпізнавання CLICK, DOUBLE, LONG та повторних HOLD без блокувань.
    switch (state) {
      case IDLE:
        if (down) { 
          state = DOWN1; 
          t = now; 
        }
        break;
        
      case DOWN1:
        if (up) { 
          state = WAIT2; 
          t = now; 
        }
        else if (stable && (now - t >= LONG_PRESS_MS)) {
          emit(Gesture::LONG);
          state = HELD;
          lastRepeat = now;
        }
        break;
        
      case WAIT2:
        if (down && (now - t <= DOUBLE_CLICK_MS)) { 
          state = DOWN2; 
          t = now; 
        }
        else if (now - t >= DOUBLE_CLICK_MS) {
          emit(Gesture::CLICK);
          state = IDLE;
        }
        break;
        
      case DOWN2:
        if (up) { 
          emit(Gesture::DOUBLE); 
          state = IDLE; 
        }
        else if (stable && (now - t >= LONG_PRESS_MS)) {
          emit(Gesture::LONG);
          state = HELD;
          lastRepeat = now;
        }
        break;
        
      case HELD:
        if (up) {
          state = IDLE;
        }
        break;
    }

    vTaskDelay(pdMS_TO_TICKS(POLL_MS));
  }
}

void buttonBegin() {
  pinMode(BUTTON_PIN, BUTTON_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
  queue = xQueueCreate(16, sizeof(Gesture));
  // Пріоритет 2 — вище за loop() (1): кнопка реагує, навіть якщо основний цикл зайнятий.
  xTaskCreate(buttonTask, "button", 3072, nullptr, 2, &task);
}

bool buttonGetGesture(Gesture& g, TickType_t wait) {
  return xQueueReceive(queue, &g, wait) == pdTRUE;
}

const char* gestureName(Gesture g) {
  switch (g) {
    case Gesture::CLICK:    return "CLICK";
    case Gesture::DOUBLE:   return "DOUBLE";
    case Gesture::LONG:     return "LONG";
    case Gesture::HOLD:     return "HOLD";
    case Gesture::LONG_END: return "LONG_END";
  }
  return "?";
}

UBaseType_t buttonStackFree() { return task ? uxTaskGetStackHighWaterMark(task) : 0; }
