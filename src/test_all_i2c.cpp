// ============================================================
//  TEST TỔNG HỢP 4 CẢM BIẾN I2C - src/test_all_i2c.cpp
//
//  SƠ ĐỒ NỐI DÂY:
//  SDA: 21, SCL: 22 (nối chung cho tất cả)
//  VCC: 3.3V, GND: GND
//  XSHUT Front: 16
//  XSHUT Left: 17
//  XSHUT Right: 23
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <MPU6050_light.h>

// --- CẤU HÌNH CHÂN ---
#define SDA_PIN 21
#define SCL_PIN 22

#define XSHUT_FRONT 16
#define XSHUT_LEFT  17
#define XSHUT_RIGHT 23

// --- ĐỊA CHỈ I2C MỚI ---
#define ADDR_FRONT 0x30
#define ADDR_LEFT  0x31
#define ADDR_RIGHT 0x32

// --- CÁC ĐỐI TƯỢNG CẢM BIẾN ---
VL53L0X loxFront;
VL53L0X loxLeft;
VL53L0X loxRight;
MPU6050 mpu(Wire);

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n==================================================");
    Serial.println(">>> KHOI TAO HE THONG 4 CAM BIEN I2C");
    Serial.println("==================================================");

    // 1. Tắt tất cả cảm biến VL53L0X
    pinMode(XSHUT_FRONT, OUTPUT); digitalWrite(XSHUT_FRONT, LOW);
    pinMode(XSHUT_LEFT,  OUTPUT); digitalWrite(XSHUT_LEFT,  LOW);
    pinMode(XSHUT_RIGHT, OUTPUT); digitalWrite(XSHUT_RIGHT, LOW);
    delay(100);

    // Khởi tạo I2C bus
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);
    delay(100);

    // 2. Khởi tạo từng cảm biến VL53L0X và đổi địa chỉ
    // --- BẬT LEFT ---
    Serial.println("[INFO] Dang bat VL53L0X LEFT...");
    digitalWrite(XSHUT_LEFT, HIGH);
    delay(150);
    loxLeft.setTimeout(500);
    if (!loxLeft.init()) {
        Serial.println("[FAIL] Khong the khoi tao VL53L0X LEFT!");
        while (1) delay(100);
    }
    loxLeft.setAddress(ADDR_LEFT);
    loxLeft.startContinuous();
    Serial.println("[ OK ] VL53L0X LEFT da doi dia chi thanh 0x31");

    // --- BẬT FRONT ---
    Serial.println("[INFO] Dang bat VL53L0X FRONT...");
    digitalWrite(XSHUT_FRONT, HIGH);
    delay(150);
    loxFront.setTimeout(500);
    if (!loxFront.init()) {
        Serial.println("[FAIL] Khong the khoi tao VL53L0X FRONT!");
        while (1) delay(100);
    }
    loxFront.setAddress(ADDR_FRONT);
    loxFront.startContinuous();
    Serial.println("[ OK ] VL53L0X FRONT da doi dia chi thanh 0x30");

    // --- BẬT RIGHT ---
    Serial.println("[INFO] Dang bat VL53L0X RIGHT...");
    digitalWrite(XSHUT_RIGHT, HIGH);
    delay(150);
    loxRight.setTimeout(500);
    if (!loxRight.init()) {
        Serial.println("[FAIL] Khong the khoi tao VL53L0X RIGHT!");
        while (1) delay(100);
    }
    loxRight.setAddress(ADDR_RIGHT);
    loxRight.startContinuous();
    Serial.println("[ OK ] VL53L0X RIGHT da doi dia chi thanh 0x32");

    // 3. Khởi tạo MPU6050
    Serial.println("[INFO] Dang khoi tao MPU6050...");
    byte status = mpu.begin();
    if (status != 0) {
        Serial.print("[FAIL] MPU6050 loi, ma loi: ");
        Serial.println(status);
        while (1) delay(100);
    }
    Serial.println("[ OK ] MPU6050 (0x68) ket noi thanh cong!");
    Serial.println("[INFO] Dang Calibrate MPU6050 (Giu yen mach trong 3 giay)...");
    delay(1000);
    mpu.calcOffsets();
    Serial.println("[ OK ] Calibrate hoan tat!");

    Serial.println("\n--- TAT CA CAM BIEN DA SAN SANG ---");
    delay(1000);
}

void loop() {
    // Đọc MPU6050
    mpu.update();

    // Đọc khoảng cách (chú ý: hàm này có thể block nếu sensor lỗi hoặc lỏng dây)
    uint16_t distL = loxLeft.readRangeContinuousMillimeters();
    uint16_t distF = loxFront.readRangeContinuousMillimeters();
    uint16_t distR = loxRight.readRangeContinuousMillimeters();

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 250) {
        lastPrint = millis();

        Serial.print("L: ");
        if (distL > 1200 || distL == 8190 || distL == 8191 || loxLeft.timeoutOccurred()) Serial.print("MAX  ");
        else { Serial.print(distL); Serial.print("mm "); }

        Serial.print("| F: ");
        if (distF > 1200 || distF == 8190 || distF == 8191 || loxFront.timeoutOccurred()) Serial.print("MAX  ");
        else { Serial.print(distF); Serial.print("mm "); }

        Serial.print("| R: ");
        if (distR > 1200 || distR == 8190 || distR == 8191 || loxRight.timeoutOccurred()) Serial.print("MAX  ");
        else { Serial.print(distR); Serial.print("mm "); }

        Serial.print("| Yaw: ");
        Serial.print(mpu.getAngleZ(), 1);
        Serial.println(" deg");
    }
}
