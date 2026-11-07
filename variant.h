#pragma once
// ===== Дані студента та індивідуальний варіант (з таблиці варіантів) =====
#define STUDENT_NAME    "Прізвище Ім'я"
#define STUDENT_PREFIX  "prizvyshche0101"   // прізвище латиницею + ДДММ народження
#define VARIANT_NO      1

#define GESTURE_SCHEME  1      // схема «жест → дія», 1..6
#define BLINK_PATTERN   1      // патерн режиму BLINK, 1..6
#define STATUS_CODE_N   3      // кількість спалахів (лише для патерну 6)

#define PWM_FREQ_HZ     1000   // частота ШІМ
#define PWM_RES_BITS    8      // розрядність ШІМ
#define BRIGHT_CURVE    1      // 1 — лінійна, 2 — квадратична, 3 — гамма 2.2, 4 — CIE 1931

#define DEBOUNCE_MS     20     // антибрязкіт
#define DOUBLE_CLICK_MS 250    // вікно другого натискання
#define LONG_PRESS_MS   600    // поріг довгого натискання
