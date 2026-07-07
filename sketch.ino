#include <Arduino.h>

// Thư viện Blynk
#define BLYNK_TEMPLATE_ID "TMPL6HVETwyoa"  // ID mẫu Blynk
#define BLYNK_TEMPLATE_NAME "HT Nhúng Nhóm 3"  // Tên mẫu Blynk
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"  // Mã thông báo xác thực của Blynk

#include <WiFi.h>  // Thư viện kết nối WiFi
#include <BlynkSimpleEsp32.h>  // Thư viện Blynk cho ESP32

// Thông tin WiFi
char ssid[] = "Wokwi-GUEST";  // Tên mạng WiFi
char pass[] = "";  // Mật khẩu WiFi (để trống cho Wokwi)

// Định nghĩa chân kết nối
#define PIR_PIN 12  // Chân cảm biến PIR
#define BUZZER_PIN 13  // Chân còi
#define YELLOW_LED_PIN 14  // Chân đèn LED vàng
#define RELAY1_PIN 22  // Chân relay 1
#define RELAY2_PIN 23  // Chân relay 2
#define BUTTON_MODE_PIN 25  // Chân nút bấm chuyển chế độ cảnh báo
#define BUTTON_RELAY1_PIN 26  // Chân nút bấm điều khiển relay 1
#define BUTTON_RELAY2_PIN 27  // Chân nút bấm điều khiển relay 2

// Biến trạng thái
bool alarmMode = false;  // Trạng thái chế độ cảnh báo (bật/tắt)
bool relay1State = false;  // Trạng thái relay 1 (bật/tắt)
bool relay2State = false;  // Trạng thái relay 2 (bật/tắt)
bool pirDetected = false;  // Trạng thái phát hiện chuyển động từ PIR

BlynkTimer timer;  // Bộ đếm thời gian để thực hiện các nhiệm vụ định kỳ
unsigned long lastPrintTime = 0;  // Biến để lưu thời gian in thông tin ra Serial Monitor

// Khai báo trước hàm xử lý nút bấm (định nghĩa ở dưới)
void handleButton(int buttonPin, bool &state, int virtualPin, int relayPin = -1);
void checkPIR(); 
void printStatus();
void setup() {
  Serial.begin(115200);  // Khởi tạo giao tiếp Serial với tốc độ 115200 bps
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);  // Kết nối Blynk với mã xác thực và WiFi

  // Thiết lập các chân đầu vào/đầu ra
  pinMode(PIR_PIN, INPUT);  // Cảm biến PIR là đầu vào
  pinMode(BUZZER_PIN, OUTPUT);  // Còi là đầu ra
  pinMode(YELLOW_LED_PIN, OUTPUT);  // Đèn LED vàng là đầu ra
  pinMode(RELAY1_PIN, OUTPUT);  // Relay 1 là đầu ra
  pinMode(RELAY2_PIN, OUTPUT);  // Relay 2 là đầu ra
  pinMode(BUTTON_MODE_PIN, INPUT_PULLUP);  // Nút bấm chế độ cảnh báo là đầu vào, sử dụng điện trở kéo lên (pull-up)
  pinMode(BUTTON_RELAY1_PIN, INPUT_PULLUP);  // Nút điều khiển relay 1 là đầu vào, sử dụng điện trở kéo lên
  pinMode(BUTTON_RELAY2_PIN, INPUT_PULLUP);  // Nút điều khiển relay 2 là đầu vào, sử dụng điện trở kéo lên

  // Tắt đèn và relay khi khởi động
  digitalWrite(RELAY1_PIN, LOW);  // Tắt relay 1
  digitalWrite(RELAY2_PIN, LOW);  // Tắt relay 2
   // Tắt chế độ cảnh báo khi khởi động
  alarmMode = false;  // Đảm bảo chế độ cảnh báo tắt khi khởi động

  // Đồng bộ trạng thái với ứng dụng Blynk
  Blynk.virtualWrite(V1, relay1State);  // Đồng bộ relay 1 với chân ảo V1
  Blynk.virtualWrite(V2, relay2State);  // Đồng bộ relay 2 với chân ảo V2
  Blynk.virtualWrite(V3, alarmMode);  // Đồng bộ chế độ cảnh báo với V3

  // Thiết lập bộ đếm thời gian để kiểm tra cảm biến PIR mỗi 1 giây
  timer.setInterval(1000L, checkPIR);
}

void loop() {
  Blynk.run();  // Chạy các tiến trình của Blynk
  timer.run();  // Chạy bộ đếm thời gian

  printStatus();  // In trạng thái các thiết bị ra Serial Monitor

  // Xử lý các nút bấm để thay đổi trạng thái thiết bị
  handleButton(BUTTON_MODE_PIN, alarmMode, V3);  // Nút bấm chuyển chế độ cảnh báo
  handleButton(BUTTON_RELAY1_PIN, relay1State, V1, RELAY1_PIN);  // Nút bấm điều khiển relay 1
  handleButton(BUTTON_RELAY2_PIN, relay2State, V2, RELAY2_PIN);  // Nút bấm điều khiển relay 2
}

// Hàm xử lý nút bấm
// buttonPin: chân nút bấm, state: biến trạng thái, virtualPin: chân ảo Blynk, relayPin: chân điều khiển relay (nếu có)
void handleButton(int buttonPin, bool &state, int virtualPin, int relayPin) {
  if (digitalRead(buttonPin) == LOW) {  // Nếu nút bấm được nhấn
    state = !state;  // Đảo trạng thái (bật/tắt)
    Blynk.virtualWrite(virtualPin, state);  // Đồng bộ trạng thái với ứng dụng Blynk
    if (relayPin != -1) {  // Nếu có chân relay
      digitalWrite(relayPin, state ? HIGH : LOW);  // Điều khiển relay
    }
    delay(300);  // Chờ 300ms để chống nhấn lặp lại (debounce)
  }
}

// Hàm in trạng thái các thiết bị ra Serial Monitor
void printStatus() {
  if (millis() - lastPrintTime >= 1000) {  // Kiểm tra nếu đã qua 1 giây
    Serial.print("- Đèn Đỏ: ");  // In trạng thái relay 1
    Serial.println(relay1State ? "ON" : "OFF");

    Serial.print("  Đèn Xanh: ");  // In trạng thái relay 2
    Serial.println(relay2State ? "ON" : "OFF");

    Serial.print("  Alarm Mode: ");  // In trạng thái chế độ cảnh báo
    Serial.println(alarmMode ? "ON" : "OFF");

    lastPrintTime = millis();  // Cập nhật thời gian in lần cuối
  }
}

// Hàm kiểm tra cảm biến PIR
void checkPIR() {
  pirDetected = digitalRead(PIR_PIN) == HIGH;  // Kiểm tra nếu có phát hiện chuyển động từ PIR

  if (pirDetected && alarmMode) {  // Nếu có chuyển động và chế độ cảnh báo đang bật
    tone(BUZZER_PIN, 2750);  // Bật còi với tần số 2750 Hz
    digitalWrite(YELLOW_LED_PIN, HIGH);  // Bật đèn LED vàng
    Blynk.virtualWrite(V4, 1);  // Đồng bộ chân ảo V4 khi còi kêu
    Serial.println("Phát hiện chuyển động! Còi hú, đèn vàng bật");  // In thông báo phát hiện chuyển động
    delay(5000);  // Chờ 5 giây trước khi tắt còi
    noTone(BUZZER_PIN);  // Tắt còi
    digitalWrite(YELLOW_LED_PIN, LOW);  // Tắt đèn LED vàng
    Blynk.virtualWrite(V4, 0);  // Tắt chân ảo V4 khi còi ngừng
  } else {
    digitalWrite(YELLOW_LED_PIN, LOW);  // Nếu không phát hiện chuyển động, tắt đèn LED vàng
  }
}

// Điều khiển relay 1 qua chân ảo V1
BLYNK_WRITE(V1) {
  relay1State = param.asInt();  // Nhận trạng thái từ ứng dụng
  digitalWrite(RELAY1_PIN, relay1State ? HIGH : LOW);  // Điều khiển relay 1
}

// Điều khiển relay 2 qua chân ảo V2
BLYNK_WRITE(V2) {
  relay2State = param.asInt();  // Nhận trạng thái từ ứng dụng
  digitalWrite(RELAY2_PIN, relay2State ? HIGH : LOW);  // Điều khiển relay 2
}

// Điều khiển chế độ cảnh báo qua chân ảo V3
BLYNK_WRITE(V3) {
  alarmMode = param.asInt();  // Nhận trạng thái từ ứng dụng
}
