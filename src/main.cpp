// ============================================================
//  TEST ĐỘC LẬP VL53L0X LEFT - src/main.cpp
//
//  SƠ ĐỒ NỐI DÂY CẢM BIẾN TRÁI (LEFT) <-> ESP32:
//  --------------------------------------------------
//  | Chân VL53L0X | Nối vào ESP32 | Ghi chú         |
//  |:------------:|:-------------:|:----------------|
//  | VIN / VCC    | 3.3V          | Cấp nguồn 3.3V  |
//  | GND          | GND           | Nối chung Mass  |
//  | SCL          | GPIO 22       | Xung nhịp I2C   |
//  | SDA          | GPIO 21       | Dữ liệu I2C     |
//  | XSHUT        | GPIO 17       | Bật/tắt cảm biến|
//  --------------------------------------------------
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

// --- CẤU HÌNH CHÂN ---
#define SDA_PIN      21
#define SCL_PIN      22

#define XSHUT_FRONT  16
#define XSHUT_LEFT   17
#define XSHUT_RIGHT  23

VL53L0X loxLeft;

// ============================================================
//  HÀM HỖ TRỢ
// ============================================================
void printHeader(const char* title) {
    Serial.println("\n==================================================");
    Serial.print(">>> ");
    Serial.println(title);
    Serial.println("==================================================");
}

void printSeparator() {
    Serial.println("--------------------------------------------------");
}

void scanI2C() {
    Serial.println("[INFO] Dang quet I2C Bus (dia chi 0x01 -> 0x7F)...");
    byte error, address;
    int nDevices = 0;

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("[ OK ] Tim thay thiet bi tai 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);

            if (address == 0x29) Serial.println("  <-- VL53L0X (Mac dinh)");
            else Serial.println("  <-- Thiet bi khac");

            nDevices++;
        } else if (error == 4) {
            Serial.print("[WARN] Loi khong xac dinh tai 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
        }
    }

    if (nDevices == 0) {
        Serial.println("[FAIL] KHONG TIM THAY THIET BI NAO TREN BUS I2C!");
    } else {
        Serial.print("[INFO] Tong so thiet bi tim thay: ");
        Serial.println(nDevices);
    }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(2000);  // Chờ Serial Monitor mở

    printHeader("TEST DOC LAP VL53L0X LEFT (Trai)");

    // --- BUOC 0: Reset tat ca cac cam bien ---
    Serial.println("[INFO] Dang reset tat ca cac cam bien (keo XSHUT xuong LOW)...");
    pinMode(XSHUT_FRONT, OUTPUT); digitalWrite(XSHUT_FRONT, LOW);
    pinMode(XSHUT_LEFT,  OUTPUT); digitalWrite(XSHUT_LEFT,  LOW);
    pinMode(XSHUT_RIGHT, OUTPUT); digitalWrite(XSHUT_RIGHT, LOW);
    delay(100);

    // --- BUOC 1: Khoi tao I2C ---
    Wire.begin(SDA_PIN, SCL_PIN);
    pinMode(SDA_PIN, INPUT_PULLUP);
    pinMode(SCL_PIN, INPUT_PULLUP);
    Wire.setClock(100000); // 100kHz - chuẩn an toàn cho I2C
    Wire.setTimeout(1000);
    delay(100);

    // --- BUOC 2: Bat cam bien LEFT ---
    Serial.println("[INFO] Dang bat cam bien LEFT (GPIO 17 -> HIGH)...");
    digitalWrite(XSHUT_LEFT, HIGH);
    delay(150); // Chờ cảm biến boot hoàn toàn (VL53L0X cần khoảng 1.2ms, delay 150ms cho chắc chắn)

    // --- BUOC 3: Quet I2C ---
    printHeader("BUOC 1: Quet I2C Bus");
    scanI2C();

    // --- BUOC 4: Khoi tao VL53L0X ---
    printHeader("BUOC 2: Khoi tao VL53L0X LEFT (Thu vien Pololu)");
    loxLeft.setTimeout(500);
    if (!loxLeft.init()) {
        Serial.println("[FAIL] Khoi tao VL53L0X LEFT THAT BAI!");
        Serial.println("       1. Kiem tra lai day SDA, SCL, VCC, GND.");
        Serial.println("       2. Kiem tra XSHUT co cam dung chan 17 khong.");
        while (1) delay(100);
    }
    
    // Tự động đo liên tục
    loxLeft.startContinuous();

    Serial.println("[ OK ] VL53L0X LEFT khoi tao thanh cong!");
    printHeader("BUOC 3: Doc khoang cach lien tuc (mm)");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
    uint16_t distance = loxLeft.readRangeContinuousMillimeters();
    
    Serial.print("LEFT [mm]: ");
    if (loxLeft.timeoutOccurred()) {
        Serial.println("TIMEOUT (Loi giao tiep, kiem tra lai day!)");
    } else if (distance > 1200 || distance == 8190 || distance == 8191) { 
        // Thư viện Pololu trả về 8190/8191 nếu ra ngoài tầm quét (out of range)
        Serial.println("OUT_OF_RANGE (Khong co vat can)");
    } else {
        Serial.println(distance);
    }

    delay(200); // Đọc khoảng cách mỗi 200ms
}