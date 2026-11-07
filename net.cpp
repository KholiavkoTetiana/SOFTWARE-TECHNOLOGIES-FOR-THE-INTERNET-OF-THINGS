#include "net.h"
#include "variant.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ---------- Джерело даних з інтернету (варіант, табл. 2.16 методички) ----------
// TODO (Л2, завдання 5): підставте адресу, назву й одиницю свого джерела з таблиці 2.16.
// Довгу адресу розбивайте на кілька рядків у лапках — компілятор склеїть їх сам.
#define API_TITLE "Lviv temp"                      // приклад: джерело 1
#define API_UNITS "C"
#define API_LINK  "https://api.open-meteo.com/v1/forecast" \
                  "?latitude=49.84&longitude=24.03&current=temperature_2m"

// Витягнути потрібне число з розібраного документа. Повертає false, якщо потрібного
// поля немає (сервер відповів, але не тим, чого ми чекали).
static bool apiExtract(JsonDocument& doc, float& out) {
  // TODO (Л2, завдання 5): шлях до потрібного поля — у таблиці 2.16.
  //   об'єкт у об'єкті:  doc["current"]["temperature_2m"]
  //   масив у корені:    doc[0]["rate"]
  //   елемент масиву:    doc["result"]["XETHZUSD"]["c"][0]
  //   якщо число передане рядком, as<float>() перетворить його сам;
  //   якщо потрібні інші одиниці (секунди → години), перерахуйте тут.
  JsonVariant v = doc["current"]["temperature_2m"];   // приклад: джерело 1
  if (v.isNull()) return false;
  out = v.as<float>();
  return true;
}

// ---------- Час ----------
// Рядок часового поясу у форматі POSIX: стандартний час EET = UTC+2,
// літній EEST з останньої неділі березня 03:00 до останньої неділі жовтня 04:00.
static const char* TZ_KYIV = "EET-2EEST,M3.5.0/3,M10.5.0/4";
static const char* NTP_1 = "pool.ntp.org";
static const char* NTP_2 = "time.google.com";
static const time_t TIME_SANE = 1700000000;   // 2023-11-14: менші значення — годинник ще не йде

// ---------- Стан ----------
static NetState state = NetState::IDLE;
static uint32_t stateSince = 0;
static uint32_t reconnects = 0;
static bool timeOk = false;
static bool ntpStarted = false;
static char ipText[16] = "0.0.0.0";

static const uint32_t CONNECT_TIMEOUT_MS = 15000;  // скільки чекати на підключення
static const uint32_t RETRY_WAIT_MS      = 5000;   // пауза перед наступною спробою

// ---------- Задача запитів ----------
static TaskHandle_t fetchTask = nullptr;
static SemaphoreHandle_t apiMutex = nullptr;
static ApiResult api = {};
static uint32_t lastFetchMs = 0;
// volatile: прапорець пише loop(), а скидає задача fetch
static volatile bool fetchBusy = false;

static void storeResult(const ApiResult& r) {
  xSemaphoreTake(apiMutex, portMAX_DELAY);
  api = r;
  xSemaphoreGive(apiMutex);
}

// Один запит: TLS-з'єднання, GET, розбір JSON. Виконується лише в задачі fetchTask,
// бо триває секунди — у loop() він зупинив би світлодіод і кнопку.
static void doFetch() {
  ApiResult r;
  xSemaphoreTake(apiMutex, portMAX_DELAY);
  r = api;
  xSemaphoreGive(apiMutex);
  r.seq++;
  r.ok = false;
  r.error[0] = '\0';

  uint32_t t0 = millis();
  WiFiClientSecure client;
  client.setInsecure();          // без перевірки сертифіката: лише для навчання, див. п. 2.1.3
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  http.setUserAgent("ptir-" STUDENT_PREFIX);

  if (!http.begin(client, API_LINK)) {
    snprintf(r.error, sizeof(r.error), "begin failed");
    r.httpCode = 0;
  } else {
    r.httpCode = http.GET();
    // TODO (Л2, завдання 5): обробіть усі випадки і запишіть текст у r.error:
    //   - код 200: прочитати тіло через http.getString() (НЕ getStream() — див. п. 2.1.4),
    //     розібрати deserializeJson(), перевірити DeserializationError,
    //     викликати apiExtract(); у разі успіху: r.ok = r.valid = true, r.atMs = millis();
    //   - код > 0, але не 200: відповів сервер, помилка на його боці або в запиті;
    //   - код < 0: до сервера не дійшли — текст дає HTTPClient::errorToString().
    if (r.httpCode == HTTP_CODE_OK) {
      String body = http.getString();
      JsonDocument doc;
      if (!deserializeJson(doc, body) && apiExtract(doc, r.value)) {
        r.ok = true;
        r.valid = true;
        r.atMs = millis();
      } else {
        snprintf(r.error, sizeof(r.error), "parse");
      }
    } else {
      snprintf(r.error, sizeof(r.error), "http %d", r.httpCode);
    }
    http.end();
  }

  r.durationMs = millis() - t0;
  if (r.ok) r.okCount++; else r.failCount++;
  storeResult(r);
}

static void fetchTaskBody(void*) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   // спимо, поки loop() не попросить запит
    doFetch();
    fetchBusy = false;
  }
}

// ---------- Публічний інтерфейс ----------
void netBegin() {
  apiMutex = xSemaphoreCreateMutex();
  memset(&api, 0, sizeof(api));
  // 8 КБ стеку: рукостискання TLS (mbedTLS) — найбільший споживач стеку в проєкті.
  xTaskCreate(fetchTaskBody, "fetch", 8192, nullptr, 1, &fetchTask);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);   // перепідключенням керує наш автомат, а не ядро
  WiFi.setSleep(false);
  state = NetState::IDLE;
  stateSince = millis();
}

// TODO (Л2, завдання 2): повний автомат підключення до Wi-Fi (п. 2.1.1 методички).
//   IDLE       -> WiFi.begin(), журнал "WIFI: CONNECTING ssid=…", далі CONNECTING
//   CONNECTING -> підключилися: запам'ятати IP, запустити configTzTime(), журнал "WIFI: OK …",
//                 дозволити перший запит; минув CONNECT_TIMEOUT_MS: "WIFI: FAIL timeout" -> WAITING
//   ONLINE     -> зв'язок зник: reconnects++, "WIFI: LOST n=…" -> WAITING
//   WAITING    -> минув RETRY_WAIT_MS -> IDLE (нова спроба)
// TODO (Л2, завдання 3): у стані ONLINE визначити момент, коли годинник пішов (без блокування!),
//   виставити timeOk і надрукувати рядок "TIME: <РРРР-ММ-ДД ГГ:ХХ:СС ПОЯС>".
// Заглушка нижче лише підключається: немає ні тайм-ауту, ні повторів, ні обробки обриву.
void netUpdate(uint32_t now) {
  switch (state) {
    case NetState::IDLE:
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      state = NetState::CONNECTING;
      stateSince = now;
      Serial.printf("WIFI: CONNECTING ssid=%s\n", WIFI_SSID);
      break;

    case NetState::CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        uint32_t took = now - stateSince;
        strncpy(ipText, WiFi.localIP().toString().c_str(), sizeof(ipText) - 1);
        state = NetState::ONLINE;
        stateSince = now;
        Serial.printf("WIFI: OK ip=%s rssi=%d ch=%d t=%lums\n", ipText, WiFi.RSSI(),
                      WiFi.channel(), (unsigned long)took);
        lastFetchMs = now - (uint32_t)API_PERIOD_S * 1000;
      }
      break;

    case NetState::ONLINE:
      if (!fetchBusy && now - lastFetchMs >= (uint32_t)API_PERIOD_S * 1000) netFetchNow();
      break;

    case NetState::WAITING:
      break;
  }
}

bool netOnline() { return state == NetState::ONLINE; }
NetState netState() { return state; }

const char* netStateName() {
  switch (state) {
    case NetState::IDLE:       return "IDLE";
    case NetState::CONNECTING: return "CONNECTING";
    case NetState::ONLINE:     return "ONLINE";
    case NetState::WAITING:    return "WAITING";
  }
  return "?";
}

int netRssi() { return netOnline() ? WiFi.RSSI() : 0; }
const char* netIp() { return ipText; }
uint32_t netReconnects() { return reconnects; }
bool netTimeValid() { return timeOk; }

bool netLocalTime(struct tm& out) {
  if (!timeOk) return false;
  time_t now = time(nullptr);
  localtime_r(&now, &out);
  return true;
}

void netClock(char* buf, size_t len) {
  struct tm t;
  if (netLocalTime(t)) strftime(buf, len, "%H:%M:%S", &t);
  else strncpy(buf, "--:--:--", len);
}

void netFetchNow() {
  if (fetchBusy || !fetchTask) return;
  fetchBusy = true;
  lastFetchMs = millis();
  xTaskNotifyGive(fetchTask);
}

bool netApi(ApiResult& out) {
  if (!apiMutex) return false;
  xSemaphoreTake(apiMutex, portMAX_DELAY);
  out = api;
  xSemaphoreGive(apiMutex);
  return true;
}

const char* apiName() { return API_TITLE; }
const char* apiUnit() { return API_UNITS; }
const char* apiUrl()  { return API_LINK; }
UBaseType_t netStackFree() { return fetchTask ? uxTaskGetStackHighWaterMark(fetchTask) : 0; }
