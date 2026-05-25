// ============================================================
//  TEST TỔNG HỢP: 3x VL53L0X + MPU6050
//  File: src/main.cpp
//  Env:  test_vl53l0x_front (platformio.ini)
//
//  SƠ ĐỒ NỐI DÂY:
//  ┌────────────────┬──────────┬─────────────────────┐
//  │  Linh kiện     │  Pin     │  ESP32 GPIO         │
//  ├────────────────┼──────────┼─────────────────────┤
//  │  Tất cả        │  SDA     │  GPIO 21            │
//  │  Tất cả        │  SCL     │  GPIO 22            │
//  │  Tất cả        │  VCC     │  3.3V               │
//  │  Tất cả        │  GND     │  GND                │
//  ├────────────────┼──────────┼─────────────────────┤
//  │  VL53L0X FRONT │  XSHUT   │  GPIO 16            │
//  │  VL53L0X LEFT  │  XSHUT   │  GPIO 17            │
//  │  VL53L0X RIGHT │  XSHUT   │  GPIO 23            │
//  ├────────────────┼──────────┼─────────────────────┤
//  │  MPU6050       │  AD0     │  GND (addr=0x68)    │
//  └────────────────┴──────────┴─────────────────────┘
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <MPU6050_light.h>

// ─── PIN CONFIG ──────────────────────────────────────────────
#define SDA_PIN      21
#define SCL_PIN      22
#define XSHUT_FRONT  16
#define XSHUT_LEFT   17
#define XSHUT_RIGHT  23

// Địa chỉ I2C sau khi đổi
#define ADDR_FRONT   0x30
#define ADDR_LEFT    0x31
#define ADDR_RIGHT   0x32

// ─── OBJECTS ─────────────────────────────────────────────────
VL53L0X  loxFront;
VL53L0X  loxLeft;
VL53L0X  loxRight;
MPU6050  mpu(Wire);

// ─── TRẠNG THÁI INIT ─────────────────────────────────────────
bool okFront = false;
bool okLeft  = false;
bool okRight = false;
bool okMPU   = false;

// ─── HELPERS ─────────────────────────────────────────────────
void printLine(char c = '-', int n = 52) {
    for (int i = 0; i < n; i++) Serial.print(c);
    Serial.println();
}

void printHeader(const char* title) {
    Serial.println();
    printLine('=');
    Serial.print("  "); Serial.println(title);
    printLine('=');
}

// Đọc khoảng cách VL53L0X, trả về -1 nếu lỗi/out-of-range
int readDist(VL53L0X &sensor) {
    uint16_t d = sensor.readRangeContinuousMillimeters();
    if (sensor.timeoutOccurred() || d == 8190 || d == 8191 || d > 1200)
        return -1;
    return (int)d;
}

// ─── INIT VL53L0X ────────────────────────────────────────────
bool initVL53(VL53L0X &sensor, int xshutPin, uint8_t newAddr, const char* name) {
    Serial.printf("  [%s] Bat XSHUT (GPIO%d)... ", name, xshutPin);
    digitalWrite(xshutPin, HIGH);
    delay(150); // VL53L0X cần ~1.2ms để boot, 150ms cho chắc

    sensor.setTimeout(500);
    if (!sensor.init()) {
        Serial.printf("FAIL (kiem tra SDA/SCL/VCC/XSHUT)\r\n");
        return false;
    }
    sensor.setAddress(newAddr);
    sensor.startContinuous();
    Serial.printf("OK -> dia chi 0x%02X\r\n", newAddr);
    return true;
}

// ─── SETUP ───────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);

    printHeader("BOOT OK - ESP32 KHOI DONG");
    Serial.println("  Test: 3x VL53L0X (Front/Left/Right) + MPU6050");
    Serial.println("  Baud: 115200 | I2C: SDA=21 SCL=22 | 100kHz");

    // ── Bước 1: Reset tất cả XSHUT xuống LOW ─────────────────
    printHeader("BUOC 1: Reset XSHUT tat ca cam bien");
    pinMode(XSHUT_FRONT, OUTPUT); digitalWrite(XSHUT_FRONT, LOW);
    pinMode(XSHUT_LEFT,  OUTPUT); digitalWrite(XSHUT_LEFT,  LOW);
    pinMode(XSHUT_RIGHT, OUTPUT); digitalWrite(XSHUT_RIGHT, LOW);
    Serial.println("  Tat ca XSHUT = LOW (reset xong)");
    delay(50);

    // ── Bước 2: Khởi tạo I2C ─────────────────────────────────
    printHeader("BUOC 2: Khoi tao I2C Bus");
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);  // 100kHz — ổn định nhất
    Wire.setTimeout(1000);  // Timeout tránh treo bus
    delay(50);
    // Pull-up nội bộ dự phòng (nên có 4.7kΩ ngoài)
    pinMode(SDA_PIN, INPUT_PULLUP);
    pinMode(SCL_PIN, INPUT_PULLUP);
    Serial.println("  Wire.begin() OK | 100kHz | Timeout=1000ms");

    // ── Bước 3: Scan I2C trước khi init ──────────────────────
    printHeader("BUOC 3: Scan I2C Bus (truoc init)");
    int found = 0;
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  [FOUND] 0x%02X", addr);
            if (addr == 0x68 || addr == 0x69) Serial.print("  <- MPU6050");
            if (addr == 0x29)                 Serial.print("  <- VL53L0X (mac dinh)");
            if (addr == 0x30)                 Serial.print("  <- VL53L0X FRONT");
            if (addr == 0x31)                 Serial.print("  <- VL53L0X LEFT");
            if (addr == 0x32)                 Serial.print("  <- VL53L0X RIGHT");
            Serial.println();
            found++;
        }
    }
    if (found == 0)
        Serial.println("  [WARN] Khong tim thay thiet bi nao! Kiem tra day SDA/SCL.");
    else
        Serial.printf("  Tong: %d thiet bi\r\n", found);

    // ── Bước 4: Init 3x VL53L0X (lần lượt từng cái) ─────────
    printHeader("BUOC 4: Khoi tao 3x VL53L0X");
    // Thứ tự: LEFT → FRONT → RIGHT (để tránh địa chỉ 0x29 xung đột)
    okLeft  = initVL53(loxLeft,  XSHUT_LEFT,  ADDR_LEFT,  "LEFT ");
    okFront = initVL53(loxFront, XSHUT_FRONT, ADDR_FRONT, "FRONT");
    okRight = initVL53(loxRight, XSHUT_RIGHT, ADDR_RIGHT, "RIGHT");

    // ── Bước 5: Init MPU6050 ──────────────────────────────────
    printHeader("BUOC 5: Khoi tao MPU6050");
    Wire.setClock(100000); // Reset clock sau VL53L0X
    byte mpuStatus = 255;
    for (int i = 1; i <= 3; i++) {
        Serial.printf("  Lan thu %d/3... ", i);
        mpuStatus = mpu.begin();
        if (mpuStatus == 0) {
            Serial.println("OK!");
            break;
        }
        Serial.printf("FAIL (err=%d)\r\n", mpuStatus);
        delay(300);
    }

    if (mpuStatus == 0) {
        okMPU = true;
        Serial.println("  Calibrate MPU6050 - GIU YEN XE trong 3 giay...");
        for (int i = 3; i > 0; i--) {
            Serial.printf("  ... %d\r\n", i);
            delay(1000);
        }
        mpu.calcOffsets();
        Serial.println("  Calibrate XONG!");
    } else {
        Serial.println("  [FAIL] MPU6050 khong phan hoi.");
        Serial.println("         > Kiem tra: SDA=21, SCL=22, VCC=3.3V, AD0=GND");
    }

    // ── Bước 6: Scan I2C sau init — xác nhận ─────────────────
    printHeader("BUOC 6: Scan I2C (sau init - xac nhan dia chi)");
    found = 0;
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  [FOUND] 0x%02X", addr);
            if (addr == 0x68 || addr == 0x69) Serial.print("  <- MPU6050");
            if (addr == 0x30)                 Serial.print("  <- VL53L0X FRONT [OK]");
            if (addr == 0x31)                 Serial.print("  <- VL53L0X LEFT  [OK]");
            if (addr == 0x32)                 Serial.print("  <- VL53L0X RIGHT [OK]");
            Serial.println();
            found++;
        }
    }
    Serial.printf("  Tong: %d thiet bi\r\n", found);

    // ── Tổng kết trạng thái ───────────────────────────────────
    printHeader("TONG KET TRANG THAI CAM BIEN");
    Serial.printf("  VL53L0X FRONT  : %s\r\n", okFront ? "[ OK ] San sang" : "[FAIL] Loi - doc se la ERR");
    Serial.printf("  VL53L0X LEFT   : %s\r\n", okLeft  ? "[ OK ] San sang" : "[FAIL] Loi - doc se la ERR");
    Serial.printf("  VL53L0X RIGHT  : %s\r\n", okRight ? "[ OK ] San sang" : "[FAIL] Loi - doc se la ERR");
    Serial.printf("  MPU6050        : %s\r\n", okMPU   ? "[ OK ] San sang" : "[FAIL] Loi - goc se la 0.0");
    printLine('=');
    Serial.println("  Bat dau doc du lieu...");
    Serial.println("  Format: F=xxmm | L=xxmm | R=xxmm | Yaw=xx.x | Pit=xx.x | Rol=xx.x");
    printLine('=');
    delay(1000);
}

// ─── LOOP ────────────────────────────────────────────────────
void loop() {
    static unsigned long lastPrint = 0;
    const unsigned long INTERVAL = 200; // In mỗi 200ms

    // Cập nhật MPU6050 liên tục (cần gọi thường xuyên)
    if (okMPU) mpu.update();

    if (millis() - lastPrint < INTERVAL) return;
    lastPrint = millis();

    // ── Đọc 3 VL53L0X ────────────────────────────────────────
    int distF = okFront ? readDist(loxFront) : -2;
    int distL = okLeft  ? readDist(loxLeft)  : -2;
    int distR = okRight ? readDist(loxRight) : -2;

    // ── In theo định dạng bảng ────────────────────────────────
    // VL53L0X
    Serial.print("F=");
    if      (distF == -2) Serial.print("INIT_ERR ");
    else if (distF == -1) Serial.print(">1200mm  ");
    else                  Serial.printf("%-7dmm", distF);

    Serial.print(" | L=");
    if      (distL == -2) Serial.print("INIT_ERR ");
    else if (distL == -1) Serial.print(">1200mm  ");
    else                  Serial.printf("%-7dmm", distL);

    Serial.print(" | R=");
    if      (distR == -2) Serial.print("INIT_ERR ");
    else if (distR == -1) Serial.print(">1200mm  ");
    else                  Serial.printf("%-7dmm", distR);

    // MPU6050 angles
    Serial.print(" | Yaw=");
    if (okMPU) Serial.printf("%6.1f", mpu.getAngleZ());
    else       Serial.print("  N/A ");

    Serial.print(" | Pit=");
    if (okMPU) Serial.printf("%6.1f", mpu.getAngleY());
    else       Serial.print("  N/A ");

    Serial.print(" | Rol=");
    if (okMPU) Serial.printf("%6.1f", mpu.getAngleX());
    else       Serial.print("  N/A ");

    Serial.println(" deg");
}