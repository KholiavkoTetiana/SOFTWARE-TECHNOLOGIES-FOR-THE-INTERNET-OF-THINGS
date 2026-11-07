#pragma once
// ===== Дані студента та індивідуальний варіант (з таблиці варіантів) =====
#define STUDENT_NAME    "Холявко Тетяна"   // прізвище та ім'я кирилицею
#define STUDENT_PREFIX  "kholiavko0410"   // прізвище латиницею + ДДММ народження
#define VARIANT_NO      20

// ----- Лабораторна робота №1 -----
#define GESTURE_SCHEME  2      // схема «жест → дія», 1..6
#define BLINK_PATTERN   6      // патерн режиму BLINK, 1..6
#define STATUS_CODE_N   2      // кількість спалахів (лише для патерну 6)

#define PWM_FREQ_HZ     20000  // частота ШІМ
#define PWM_RES_BITS    10     // розрядність ШІМ
#define BRIGHT_CURVE    4      // 1 — лінійна, 2 — квадратична, 3 — гамма 2.2, 4 — CIE 1931

#define DEBOUNCE_MS     50     // антибрязкіт
#define DOUBLE_CLICK_MS 300    // вікно другого натискання
#define LONG_PRESS_MS   700    // поріг довгого натискання

// ----- Лабораторна робота №2 -----
#define DISPLAY_TYPE    1      // 1 — OLED SSD1306 128x64, 2 — LCD 1602 (I2C), 3 — TM1637
#define SENSOR_TYPE     1      // 1 — DHT22 темп., 2 — DHT22 волог., 3 — NTC,
                               // 4 — фоторезистор, 5 — DS18B20
#define API_SOURCE      1      // номер джерела даних з табл. 2.16 (для банера і звіту;
                               // саму адресу і шлях у JSON вписують у net.cpp)
#define API_PERIOD_S    60     // період запиту до джерела, с
#define FAIL_POLICY     1      // поведінка дисплея при помилці запиту:
                               // 1 — останнє значення зі зірочкою, 2 — прочерки, 3 — код помилки

#define WIFI_SSID        "Wokwi-GUEST"   // у симуляторі — лише ця мережа, пароль порожній
#define WIFI_PASS        ""
#define SENSOR_PERIOD_MS 2000  // період опитування датчика (DHT22 не можна частіше ніж раз на 2 с)
#define PAGE_PERIOD_MS   4000  // період автоматичного гортання сторінок дисплея
