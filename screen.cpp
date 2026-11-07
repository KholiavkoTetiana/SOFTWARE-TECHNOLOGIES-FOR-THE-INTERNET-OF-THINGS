#include "screen.h"
#include "board.h"
#include "variant.h"
#include <Wire.h>
#include <math.h>

#if DISPLAY_TYPE == 1
  #include <Adafruit_SSD1306.h>
  static Adafruit_SSD1306 oled(128, 64, &Wire, -1);
  static const uint8_t OLED_ADDR = 0x3C;
#elif DISPLAY_TYPE == 2
  #include <hd44780.h>
  #include <hd44780ioClass/hd44780_I2Cexp.h>
  static hd44780_I2Cexp lcd(0x27);
  static const uint8_t LCD_COLS = 16, LCD_ROWS = 2;
#elif DISPLAY_TYPE == 3
  #include <TM1637Display.h>
  static TM1637Display seg(SEG_CLK_PIN, SEG_DIO_PIN);
#else
  #error "DISPLAY_TYPE має бути 1..3"
#endif

static const uint8_t PAGES = 3;
static const uint32_t REDRAW_MS = 250;   // частіше перемальовувати немає сенсу

static bool ready = false;
static bool enabled = true;
static int8_t setting = -1;      // -1 — автоматичне гортання
static uint8_t page = 1;
static uint32_t lastFlip = 0, lastDraw = 0;

// Текст значення для сторінки «дані з інтернету».
// TODO (Л2, завдання 7): поведінка при невдалому запиті — за вашим варіантом (табл. 2.17):
//   FAIL_POLICY == 1 — останнє вдале значення і зірочка після числа;
//   FAIL_POLICY == 2 — прочерки;
//   FAIL_POLICY == 3 — короткий текст помилки з d.apiError.
// Не забудьте про випадок, коли жодного вдалого запиту ще не було (d.apiValid == false).
static void formatApi(const UiData& d, char* buf, size_t len) {
  if (d.apiValid) snprintf(buf, len, "%.2f", d.apiValue);
  else            snprintf(buf, len, "----");
}

// ---------------------------------------------------------------- OLED
#if DISPLAY_TYPE == 1
// TODO (Л2, завдання 6): три сторінки (п. 2.2.2, завдання 6):
//   спільний заголовок «PTIR <плата>   <годинник>» і лінія під ним;
//   1 — значення датчика великим шрифтом (setTextSize(2)) і одиниця виміру;
//   2 — стан мережі, SSID, IP, RSSI;
//   3 — назва джерела, значення (через formatApi), одиниця і вік даних у секундах.
// Пам'ятайте: спершу малюємо в буфер, потім один раз викликаємо display().
static void draw(uint8_t p, const UiData& d) {
  char buf[24];
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.printf("PTIR %-2s      %s", d.board, d.clock);
  oled.setCursor(0, 20);
  oled.printf("page %u", p);
  if (d.sensorOk) { oled.setCursor(0, 36); oled.printf("%s %.1f %s", d.sensorName, d.sensorValue, d.sensorUnit); }
  formatApi(d, buf, sizeof(buf));
  oled.setCursor(0, 50);
  oled.print(buf);
  oled.display();
}
#endif

// ---------------------------------------------------------------- LCD 1602
#if DISPLAY_TYPE == 2
static void line(uint8_t row, const char* text) {
  char buf[LCD_COLS + 1];
  snprintf(buf, sizeof(buf), "%-*s", (int)LCD_COLS, text);   // доповнити пробілами
  lcd.setCursor(0, row);
  lcd.print(buf);
}

// TODO (Л2, завдання 6): три сторінки на 16 символів у два рядки (п. 2.2.2, завдання 6).
//   Рядок доповнюйте пробілами до кінця (функція line), інакше на екрані
//   залишиться «хвіст» від попереднього, довшого напису.
static void draw(uint8_t p, const UiData& d) {
  char a[24], b[24];
  snprintf(a, sizeof(a), "page %u  %s", p, d.clock);
  if (d.sensorOk) snprintf(b, sizeof(b), "%s %.1f%s", d.sensorName, d.sensorValue, d.sensorUnit);
  else            snprintf(b, sizeof(b), "%s --.-", d.sensorName);
  line(0, a);
  line(1, b);
}
#endif

// ---------------------------------------------------------------- TM1637
#if DISPLAY_TYPE == 3
static const uint8_t DASHES[4] = { 0x40, 0x40, 0x40, 0x40 };   // чотири риски
// TODO (Л2, завдання 6): формат значення на 4 розрядах — таблиця 2.18 методички.
static void showValue(float v, bool ok) {
  if (!ok || isnan(v)) { seg.setSegments(DASHES); return; }
  seg.showNumberDec((int)constrain(lroundf(v), -999L, 9999L), false);
}

// TODO (Л2, завдання 6): 1 — значення датчика, 2 — час ГГ:ХХ із двокрапкою
//   (showNumberDecEx(hhmm, 0b01000000, true)), 3 — значення з інтернету.
static void draw(uint8_t p, const UiData& d) {
  if (p == 1)      showValue(d.sensorValue, d.sensorOk);
  else if (p == 2) seg.showNumberDec(p, false);
  else             showValue(d.apiValue, d.apiValid);
}
#endif

// ---------------------------------------------------------------- спільна частина
bool screenBegin() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
#if DISPLAY_TYPE == 1
  ready = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (ready) { oled.clearDisplay(); oled.display(); }
#elif DISPLAY_TYPE == 2
  ready = (lcd.begin(LCD_COLS, LCD_ROWS) == 0);
  if (ready) lcd.clear();
#elif DISPLAY_TYPE == 3
  seg.setBrightness(5);
  seg.clear();
  ready = true;             // у TM1637 немає зворотного зв'язку: перевірити можна лише оком
#endif
  lastFlip = lastDraw = millis();
  return ready;
}

bool screenReady() { return ready; }
uint8_t screenPages() { return PAGES; }
int8_t screenPageSetting() { return setting; }
uint8_t screenCurrentPage() { return page; }
bool screenEnabled() { return enabled; }

const char* screenPart() {
#if DISPLAY_TYPE == 1
  return "SSD1306 128x64";
#elif DISPLAY_TYPE == 2
  return "LCD 1602 I2C";
#else
  return "TM1637 4-digit";
#endif
}

void screenSetPage(int8_t p) {
  setting = (p >= 1 && p <= (int8_t)PAGES) ? p : -1;
  if (setting > 0) page = (uint8_t)setting;
  lastFlip = millis();
  lastDraw = 0;             // перемалювати негайно
}

void screenSetEnabled(bool on) {
  enabled = on;
  if (!ready) return;
#if DISPLAY_TYPE == 1
  if (!on) { oled.clearDisplay(); oled.display(); }
#elif DISPLAY_TYPE == 2
  if (on) lcd.backlight(); else { lcd.clear(); lcd.noBacklight(); }
#elif DISPLAY_TYPE == 3
  seg.setBrightness(5, on);
  if (!on) seg.clear();
#endif
  lastDraw = 0;
}

void screenUpdate(uint32_t now, const UiData& d) {
  if (!ready || !enabled) return;
  if (setting < 0 && now - lastFlip >= PAGE_PERIOD_MS) {
    page = page % PAGES + 1;
    lastFlip = now;
    lastDraw = 0;
  }
  if (lastDraw && now - lastDraw < REDRAW_MS) return;
  lastDraw = now ? now : 1;
  draw(page, d);
}
