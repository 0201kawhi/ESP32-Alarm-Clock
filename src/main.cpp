#include <WiFi.h>
#include "time.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <Keypad.h>


#define I2C_SDA 21
#define I2C_SCL 22
// --- Wi-Fi & NTP ---
const char* ssid       = "Wokwi-GUEST";
const char* password   = "";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800; // UTC+8
const int   daylightOffset_sec = 0;

// --- OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- DHT22 ---
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float currentTemp = 0.0;
unsigned long lastDHTRead = 0; // 用於非阻塞式讀取溫度的計時器

// --- 硬體腳位 ---
#define BUZZER_PIN 5
#define BUTTON_PIN 18

// --- 鬧鐘與狀態機 ---
int alarmHour = 7;
int alarmMinute = 30;
volatile bool isAlarmRinging = false; // volatile 確保 ISR 安全

// 定義系統狀態：一般時鐘模式、設定鬧鐘模式
enum State { NORMAL, SET_ALARM };
State currentState = NORMAL;
String inputBuffer = ""; // 儲存鍵盤輸入的時間字串

// --- 4x4 矩陣鍵盤 ---
const byte ROWS = 4; 
const byte COLS = 4; 
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
// 注意：請根據你在 Wokwi 的實際接線修改以下腳位
byte rowPins[ROWS] = {32, 33, 25, 26}; // R1, R2, R3, R4
byte colPins[COLS] = {14, 12, 13, 27}; // C1, C2, C3, C4
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- 中斷服務常式 (ISR) ---
void IRAM_ATTR stopAlarmISR() {
  isAlarmRinging = false;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  // 初始化實體按鈕與中斷
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), stopAlarmISR, FALLING);

  Serial.println("\n--- BUTTON_PIN ---");

  // 初始化蜂鳴器
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("\n--- BUZZER_PIN ---");

  // 初始化 OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  Serial.println("\n--- OLED ---");

  // 初始化 DHT
  dht.begin();

  Serial.println("\n--- DHT ---");

  // 顯示連線畫面
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.println("Connecting Wi-Fi...");
  display.display();

  // 連線 Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  Serial.println("\n--- WiFi ---");

  // 對時
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  // 1. 獲取時間 (設定 10ms timeout，避免網路延遲卡死迴圈)
  struct tm timeinfo;
  bool timeValid = getLocalTime(&timeinfo, 10);

  // 2. 非阻塞式讀取溫度 (每 2000 毫秒讀一次，避免 DHT 拖慢迴圈速度)
  if (millis() - lastDHTRead > 2000) {
    currentTemp = dht.readTemperature();
    lastDHTRead = millis();
  }

  // 3. 處理鍵盤輸入與狀態轉換
  char key = keypad.getKey();
  if (key) {
    if (currentState == NORMAL) {
      if (key == 'A') { 
        // 按 A 進入設定模式
        currentState = SET_ALARM;
        inputBuffer = "";
      } else if (isAlarmRinging) {
        // 如果鬧鐘在響，按鍵盤任一鍵也可關閉 (作為 ISR 的備用)
        isAlarmRinging = false;
      }
    } 
    else if (currentState == SET_ALARM) {
      if (key >= '0' && key <= '9' && inputBuffer.length() < 4) {
        inputBuffer += key; // 允許輸入最多 4 位數字 (HHMM)
      } else if (key == 'C') {
        inputBuffer = ""; // 按 C 清除並重新輸入
      } else if (key == '*') {
        currentState = NORMAL; // 按 * 取消設定，退回主畫面
      } else if (key == '#') {
        // 按 # 儲存設定
        if (inputBuffer.length() == 4) {
          int h = inputBuffer.substring(0, 2).toInt();
          int m = inputBuffer.substring(2, 4).toInt();
          // 簡單的時間格式驗證
          if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
            alarmHour = h;
            alarmMinute = m;
          }
        }
        currentState = NORMAL;
      }
    }
  }

  // 4. 鬧鐘觸發邏輯 (準點第 0 秒觸發)
  if(currentState == NORMAL && timeValid) {
    if(timeinfo.tm_hour == alarmHour && timeinfo.tm_min == alarmMinute && timeinfo.tm_sec == 0) {
      isAlarmRinging = true;
    }
  }

  // 控制蜂鳴器硬體狀態
  if (isAlarmRinging) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // 5. 更新 OLED 顯示畫面
  display.clearDisplay();
  
  if (currentState == NORMAL) {
    // --- 一般時鐘畫面 ---
    display.setTextSize(2);
    display.setCursor(0, 0);
    if (timeValid) {
      display.printf("%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
      display.print("Syncing...");
    }
    
    // 顯示溫度
    display.setTextSize(1);
    display.setCursor(0, 30);
    if (isnan(currentTemp)) {
      display.println("Temp Error");
    } else {
      display.printf("Temp: %.1f C\n", currentTemp);
    }
    
    // 顯示鬧鐘設定與提示
    display.setCursor(0, 45);
    display.printf("Alarm: %02d:%02d ", alarmHour, alarmMinute);
    if (isAlarmRinging) {
      display.print("[RING!]");
    } else {
      display.print("[A:Set]"); // 提示操作者按 A 進行設定
    }
  } 
  else if (currentState == SET_ALARM) {
    // --- 鬧鐘設定畫面 ---
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Set Alarm (HHMM):");
    
    // 顯示輸入的字串，並用底線 "_" 提示剩餘位數
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.print(inputBuffer);
    for(int i = inputBuffer.length(); i < 4; i++) display.print("_");
    
    // 顯示操作提示
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print("#:Save  *:Cancel");
  }

  display.display();
}
