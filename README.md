
---

```markdown
# ESP32 溫濕度鬧鐘 (ESP32 Alarm Clock)

這是一個基於 ESP32 的多功能智能鬧鐘專案。透過連接 Wi-Fi 自動獲取 NTP 網路時間，確保時間絕對精準。系統整合了 OLED 顯示螢幕、DHT22 溫濕度感測器、4x4 矩陣鍵盤與實體按鈕，並採用「State Machine」與「Non-blocking」架構開發，確保系統具備極高的即時反應能力。

## ✨ 功能特色

* **🌐 網路對時 (NTP)：** 開機自動連線 Wi-Fi 並同步 UTC+8 台灣標準時間。
* **🌡️ 環境監測：** 整合 DHT22 感測器，即時顯示當下室內溫度。
* **⌨️ 矩陣鍵盤設定介面：** 透過 4x4 薄膜鍵盤，直覺地輸入數字來設定鬧鐘時間，支援錯誤清除與取消功能。
* **⚡ 零延遲中斷控制：** 具備實體硬體中斷 (ISR) 按鈕，鬧鐘響起時按下即可瞬間靜音，不受主程式迴圈影響。

---

## 系統配置圖
[專案硬體接線與模擬圖](simulator.png)
## 🛠️ 元件清單

* ESP32 開發板 
* 0.96 吋 OLED 螢幕 (SSD1306, I2C 介面)
* DHT22 溫濕度感測器
* 4x4 矩陣鍵盤 (Membrane Keypad)
* 蜂鳴器 (Buzzer)
* 輕觸開關按鈕 (Push Button)

---


## 🔌 腳位接線對應表

請依照下方表格將各元件連接至 ESP32：

| 元件名稱 | 元件腳位 | ESP32 腳位 | 說明 |
| :--- | :--- | :--- | :--- |
| **OLED (SSD1306)** | SDA | **GPIO 21** | I2C 資料線 |
| | SCL | **GPIO 22** | I2C 時脈線 |
| **DHT22** | SDA / Data | **GPIO 4** | 溫度資料讀取 |
| **Buzzer** | 正極 (+) | **GPIO 5** | 鬧鈴輸出 |
| **Push Button** | 端子 1 | **GPIO 18** | 中斷按鈕 (內部上拉) |
| | 端子 2 | **GND** | 接地 |
| **4x4 Keypad** | R1 ~ R4 | **32, 33, 25, 26** | 鍵盤橫列 (Rows) |
| | C1 ~ C4 | **14, 12, 13, 27** | 鍵盤直行 (Cols) |

*(註：OLED、DHT22、Buzzer 電源腳位請分別接至 ESP32 的 3.3V 與 GND)*

---

## 💻 開發環境與軟體設定

1. 安裝 VSCode 與 PlatformIO 擴充套件和 [Wokwi](https://wokwi.com) 模擬器。
2. 複製本專案程式碼。
3. 在專案根目錄的 `platformio.ini` 中，確認已加入以下依賴函式庫 (Library Dependencies)：

```ini
[env:featheresp32]
platform = espressif32
board = featheresp32
framework = arduino
lib_deps = 
	adafruit/Adafruit SSD1306@^2.5.16
	adafruit/Adafruit GFX Library@^1.12.6
	adafruit/DHT sensor library@^1.4.7
	chris--a/Keypad@^3.1.1

```
4. 進行編譯。

---

## 🕹️ 操作說明

### 系統狀態

開機後，OLED 螢幕將顯示目前時間、溫度，並在下方提示 `[A:Set]`，代表處於 **待機模式 (NORMAL)**。

### 設定鬧鐘

1. 按下鍵盤上的 **`A`** 鍵進入 **設定模式 (SET_ALARM)**。
2. 畫面會提示 `Set Alarm (HHMM):`，請依序輸入 4 位數字 (24小時制，例如 `0730` 代表早上 7 點半)。
3. 操作熱鍵：
* **`C`**：清除已輸入的數字，重新輸入。
* **`*`**：取消設定，直接退回待機畫面。
* **`#`**：確認並儲存時間，退回待機畫面。



### 關閉鬧鈴

當到達設定時間時，蜂鳴器會響起，螢幕提示 `[RING!]`。您可以透過以下兩種方式關閉：

1. **實體按鈕**：按下連接在 GPIO 18 的按鈕，透過硬體中斷瞬間關閉。
2. **鍵盤快速鍵**：按下 4x4 鍵盤上的 **任意鍵** 亦可解除鬧鈴。

---
