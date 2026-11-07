#pragma once
// ===== Дані студента та індивідуальний варіант (з таблиці варіантів) =====
#define STUDENT_NAME    "Холявко Тетяна"   // прізвище та ім'я кирилицею
#define STUDENT_PREFIX  "kholiavko0410"   // прізвище латиницею + ДДММ народження
#define VARIANT_NO      20

#define GESTURE_SCHEME  2      // схема «жест → дія», 1..6
#define BLINK_PATTERN   6      // патерн режиму BLINK, 1..6
#define STATUS_CODE_N   2      // кількість спалахів (лише для патерну 6)

#define PWM_FREQ_HZ     20000  // частота ШІМ
#define PWM_RES_BITS    10     // розрядність ШІМ
#define BRIGHT_CURVE    4      // 1 — лінійна, 2 — квадратична, 3 — гамма 2.2, 4 — CIE 1931

#define DEBOUNCE_MS     50     // антибрязкіт
#define DOUBLE_CLICK_MS 300    // вікно другого натискання
#define LONG_PRESS_MS   700    // поріг довгого натискання
