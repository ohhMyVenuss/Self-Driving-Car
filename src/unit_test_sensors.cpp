// ============================================================
//  UNIT TEST STATIC BENCH - src/unit_test_sensors.cpp
//
//  Mục đích: Test toàn bộ cảm biến (MPU6050 + 3x VL53L0X)
//  mà KHÔNG cần bánh xe / motor. Dùng bìa làm tường thật.
//
//  Phần cứng hiện có:
//    - ESP32 DevKit V1
//    - MPU6050 (SDA=21, SCL=22, AD0=GND => 0x68)
//    - VL53L0X FRONT  (XSHUT=GPIO16 => 0x30)
//    - VL53L0X LEFT   (XSHUT=GPIO17 => 0x31)
//    - VL53L0X RIGHT  (XSHUT=GPIO23 => 0x32)
//
//  Cách dùng:
//    platformio.ini -> default_envs = unit_test_sensors
//    Mở Serial Monitor @ 115200 baud
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <MPU6050_light.h>

// --- CHÂN ---
#define SDA_PIN      21
#define SCL_PIN      22
#define XSHUT_FRONT  16
#define XSHUT_LEFT   17
#define XSHUT_RIGHT  23

// --- NGƯỠNG TƯỜNG (mm) ---
// Cell size = 200mm. Robot ở tâm ô cách tường = 100mm.
// 120mm > 100mm -> bắt được tường | 120mm < 200mm -> không nhầm ô kế.
// Phải khớp với WALL_THRESHOLD_MM trong maze_solver.cpp.
#define CELL_SIZE_MM       200
#define WALL_THRESHOLD_MM  120

// --- OBJECTS ---
VL53L0X loxFront, loxLeft, loxRight;
MPU6050 mpu(Wire);

// --- TRẠNG THÁI INIT ---
bool frontOK = false, leftOK = false, rightOK = false, mpuOK = false;

// ============================================================
//  TIỆN ÍCH LOG
// ============================================================
void logSep()    { Serial.println(F("--------------------------------------------------")); }
void logHeader(const char* s) {
    Serial.println(F("\n=================================================="));
    Serial.print(F(">>> ")); Serial.println(s);
    Serial.println(F("=================================================="));
}
void logPass(const char* s) { Serial.print(F("[ PASS ] ")); Serial.println(s); }
void logFail(const char* s) { Serial.print(F("[ FAIL ] ")); Serial.println(s); }
void logInfo(const char* s) { Serial.print(F("[ INFO ] ")); Serial.println(s); }
void logWarn(const char* s) { Serial.print(F("[ WARN ] ")); Serial.println(s); }

// ============================================================
//  TEST 0: QUÉT I2C BUS
// ============================================================
void test0_scanI2C() {
    logHeader("TEST 0: QUET I2C BUS");
    int found = 0;
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  [FOUND] 0x%02X", addr);
            if      (addr == 0x68 || addr == 0x69) Serial.print("  <-- MPU6050");
            else if (addr == 0x29) Serial.print("  <-- VL53L0X (default, chua doi dia chi)");
            else if (addr == 0x30) Serial.print("  <-- VL53L0X FRONT");
            else if (addr == 0x31) Serial.print("  <-- VL53L0X LEFT");
            else if (addr == 0x32) Serial.print("  <-- VL53L0X RIGHT");
            Serial.println();
            found++;
        }
    }
    if (found == 0) logFail("Khong tim thay thiet bi nao! Kiem tra SDA/SCL/GND/VCC.");
    else            Serial.printf("  => Tim thay %d thiet bi.\n", found);
}

// ============================================================
//  TEST 1: KHỞI TẠO VL53L0X (3 cảm biến)
// ============================================================
void test1_initVL53L0X() {
    logHeader("TEST 1: KHOI TAO 3x VL53L0X");

    // Reset tất cả
    digitalWrite(XSHUT_FRONT, LOW);
    digitalWrite(XSHUT_LEFT,  LOW);
    digitalWrite(XSHUT_RIGHT, LOW);
    delay(50);
    logInfo("Da reset 3 cam bien (XSHUT=LOW).");

    // --- FRONT: boot => 0x29 => doi sang 0x30 ---
    logInfo("Bat VL53L0X FRONT (GPIO16)...");
    digitalWrite(XSHUT_FRONT, HIGH); delay(150);
    loxFront.setTimeout(500);
    if (!loxFront.init()) {
        logFail("VL53L0X FRONT khoi tao that bai! Kiem tra GPIO16, SDA/SCL, 3.3V.");
    } else {
        loxFront.setAddress(0x30);
        loxFront.startContinuous();
        frontOK = true;
        logPass("VL53L0X FRONT => OK (dia chi 0x30)");
    }

    // --- LEFT: boot => 0x29 => doi sang 0x31 ---
    logInfo("Bat VL53L0X LEFT (GPIO17)...");
    digitalWrite(XSHUT_LEFT, HIGH); delay(150);
    loxLeft.setTimeout(500);
    if (!loxLeft.init()) {
        logFail("VL53L0X LEFT khoi tao that bai! Kiem tra GPIO17, SDA/SCL, 3.3V.");
    } else {
        loxLeft.setAddress(0x31);
        loxLeft.startContinuous();
        leftOK = true;
        logPass("VL53L0X LEFT  => OK (dia chi 0x31)");
    }

    // --- RIGHT: boot => 0x29 => doi sang 0x32 ---
    logInfo("Bat VL53L0X RIGHT (GPIO23)...");
    digitalWrite(XSHUT_RIGHT, HIGH); delay(150);
    loxRight.setTimeout(500);
    if (!loxRight.init()) {
        logFail("VL53L0X RIGHT khoi tao that bai! Kiem tra GPIO23, SDA/SCL, 3.3V.");
    } else {
        loxRight.setAddress(0x32);
        loxRight.startContinuous();
        rightOK = true;
        logPass("VL53L0X RIGHT => OK (dia chi 0x32)");
    }

    logSep();
    Serial.printf("  Ket qua: FRONT=%s | LEFT=%s | RIGHT=%s\n",
        frontOK?"OK":"FAIL", leftOK?"OK":"FAIL", rightOK?"OK":"FAIL");
}

// ============================================================
//  TEST 2: KHỞI TẠO MPU6050 + CALIBRATE
// ============================================================
void test2_initMPU6050() {
    logHeader("TEST 2: KHOI TAO MPU6050");
    logInfo("Vui long GIU YEN mach 3 giay de calibrate...");

    byte status = mpu.begin();
    if (status != 0) {
        Serial.printf("  [ FAIL ] mpu.begin() tra ve loi %d\n", status);
        if (status == 1) logFail("  => NACK - MPU khong phan hoi (kiem tra SDA=21, SCL=22, AD0=GND)");
        else             logFail("  => Loi I2C khac, thu lai");
        mpuOK = false;
    } else {
        logPass("MPU6050 (0x68) ket noi thanh cong!");
        delay(1000);
        mpu.calcOffsets(); // Calibrate gyro + accel
        logPass("Calibrate hoan tat. Bat dau do goc.");
        mpuOK = true;
    }
}

// ============================================================
//  TEST 3: ĐỌC CẢM BIẾN ĐƠN THUẦN (liên tục, in ra CSV)
// ============================================================
//  Format CSV để dễ dán vào Excel/Google Sheet:
//  ms,F_mm,L_mm,R_mm,Yaw_deg,wallF,wallL,wallR
// ============================================================
void printCSVHeader() {
    Serial.println(F("\n--- CHE DO DOC CAM BIEN (nhan 's' de stop) ---"));
    Serial.println(F("ms,F_mm,L_mm,R_mm,Yaw_deg,wallF,wallL,wallR"));
}

// ============================================================
//  TEST 4: NHẬN DIỆN TƯỜNG (Wall Detection)
//  Dùng bìa đặt trước/trái/phải, kiểm tra logic nhận tường
// ============================================================
void test4_wallDetection_once() {
    if (!frontOK || !leftOK || !rightOK) {
        logWarn("Test 4 bo qua - co cam bien chua init.");
        return;
    }
    logHeader("TEST 4: NHAN DIEN TUONG (1 LAN)");

    uint16_t f = loxFront.readRangeContinuousMillimeters();
    uint16_t l = loxLeft.readRangeContinuousMillimeters();
    uint16_t r = loxRight.readRangeContinuousMillimeters();

    bool wF = !loxFront.timeoutOccurred() && f < WALL_THRESHOLD_MM && f != 8190 && f != 8191;
    bool wL = !loxLeft.timeoutOccurred()  && l < WALL_THRESHOLD_MM && l != 8190 && l != 8191;
    bool wR = !loxRight.timeoutOccurred() && r < WALL_THRESHOLD_MM && r != 8190 && r != 8191;

    Serial.printf("  FRONT: %4dmm => %s\n", f, wF ? "[ TUONG ]" : "[ Trong ]");
    Serial.printf("  LEFT : %4dmm => %s\n", l, wL ? "[ TUONG ]" : "[ Trong ]");
    Serial.printf("  RIGHT: %4dmm => %s\n", r, wR ? "[ TUONG ]" : "[ Trong ]");
    logSep();
    Serial.printf("  Wall pattern: F=%d L=%d R=%d\n", wF, wL, wR);

    // --- Kiểm tra logic rẽ Flood Fill ---
    if (!wF && !wL && !wR)  Serial.println(F("  => Logic: Di thang (khong co tuong)"));
    if (wF  && !wL && !wR)  Serial.println(F("  => Logic: TUONG PHIA TRUOC - re trai hoac phai"));
    if (!wF && wL  && !wR)  Serial.println(F("  => Logic: Tuong trai - di thang OK"));
    if (!wF && !wL && wR)   Serial.println(F("  => Logic: Tuong phai - di thang OK"));
    if (wF  && wL  && !wR)  Serial.println(F("  => Logic: Chi co duong PHAI mo"));
    if (wF  && !wL && wR)   Serial.println(F("  => Logic: Chi co duong TRAI mo"));
    if (!wF && wL  && wR)   Serial.println(F("  => Logic: Chi co di THANG (ca hai ben la tuong)"));
    if (wF  && wL  && wR)   Serial.println(F("  => Logic: BIT TU - Quay dau 180!"));
}

// ============================================================
//  TEST 5: KIỂM TRA MPU6050 GÓC YAW (nghiêng/xoay)
// ============================================================
void test5_mpuYaw_once() {
    if (!mpuOK) { logWarn("Test 5 bo qua - MPU chua init."); return; }
    logHeader("TEST 5: MPU6050 - GOC YAW (AngleZ)");

    mpu.update();
    float yaw = mpu.getAngleZ();
    float ax  = mpu.getAccX();
    float ay  = mpu.getAccY();
    float az  = mpu.getAccZ();
    float gx  = mpu.getGyroX();

    Serial.printf("  AngleZ (Yaw)  : %.2f deg\n", yaw);
    Serial.printf("  AccX/Y/Z      : %.3f / %.3f / %.3f g\n", ax, ay, az);
    Serial.printf("  GyroX         : %.2f deg/s\n", gx);

    if (abs(yaw) < 5.0) logPass("Yaw < 5 deg - Thiet bi gan nhu nam ngang tot.");
    else                logWarn("Yaw > 5 deg - MPU6050 co the bi lech hoac chua calibrate.");
}

// ============================================================
//  CHẾ ĐỘ LIVE STREAM CSV (gọi trong loop)
// ============================================================
bool streamMode = false;

void doStream() {
    if (!streamMode) return;

    // Đọc MPU
    if (mpuOK) mpu.update();

    uint16_t f = frontOK ? loxFront.readRangeContinuousMillimeters() : 9999;
    uint16_t l = leftOK  ? loxLeft.readRangeContinuousMillimeters()  : 9999;
    uint16_t r = rightOK ? loxRight.readRangeContinuousMillimeters() : 9999;

    auto valid = [](VL53L0X& lox, uint16_t v) -> String {
        if (lox.timeoutOccurred() || v == 8190 || v == 8191 || v > 2000) return "MAX";
        return String(v);
    };

    bool wF = frontOK && !loxFront.timeoutOccurred() && f < WALL_THRESHOLD_MM && f != 8190 && f != 8191;
    bool wL = leftOK  && !loxLeft.timeoutOccurred()  && l < WALL_THRESHOLD_MM && l != 8190 && l != 8191;
    bool wR = rightOK && !loxRight.timeoutOccurred() && r < WALL_THRESHOLD_MM && r != 8190 && r != 8191;

    // CSV: ms,F,L,R,Yaw,wF,wL,wR
    Serial.printf("%lu,%s,%s,%s,%.1f,%d,%d,%d\n",
        millis(),
        valid(loxFront, f).c_str(),
        valid(loxLeft,  l).c_str(),
        valid(loxRight, r).c_str(),
        mpuOK ? mpu.getAngleZ() : 0.0f,
        wF, wL, wR
    );
}

// ============================================================
//  XỬ LÝ LỆNH QUA SERIAL (Interactive CLI)
// ============================================================
void handleSerial() {
    if (!Serial.available()) return;
    char cmd = Serial.read();
    while (Serial.available()) Serial.read(); // flush

    switch (cmd) {
        case '0': test0_scanI2C();          break;
        case '1': test1_initVL53L0X();      break;
        case '2': test2_initMPU6050();      break;
        case '4': test4_wallDetection_once(); break;
        case '5': test5_mpuYaw_once();      break;
        case 's':
            streamMode = !streamMode;
            if (streamMode) printCSVHeader();
            else            logInfo("Stream DUNG.");
            break;
        case 'r':
            logInfo("RESET ESP32...");
            delay(500);
            ESP.restart();
            break;
        case 'h':
        case '?':
            Serial.println(F("\n--- MENU LENH ---"));
            Serial.println(F("  0 = Quet I2C bus"));
            Serial.println(F("  1 = Khoi tao lai 3x VL53L0X"));
            Serial.println(F("  2 = Khoi tao lai MPU6050 + Calibrate"));
            Serial.println(F("  4 = Test nhan dien tuong (1 lan)"));
            Serial.println(F("  5 = Xem goc Yaw MPU6050 (1 lan)"));
            Serial.println(F("  s = Bat/Tat stream CSV lien tuc"));
            Serial.println(F("  r = Khoi dong lai ESP32"));
            Serial.println(F("  ? = Hien thi menu nay"));
            break;
        default:
            Serial.printf("  Lenh '%c' khong hop le. Nhan '?' de xem menu.\n", cmd);
    }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(2000);

    logHeader("UNIT TEST SENSORS - KHONG CAN BANH XE");
    logInfo("ESP32 + MPU6050 + 3x VL53L0X");
    logInfo("Dung bia lam tuong de test logic nhan dien.");
    logSep();

    // Cấu hình XSHUT
    pinMode(XSHUT_FRONT, OUTPUT);
    pinMode(XSHUT_LEFT,  OUTPUT);
    pinMode(XSHUT_RIGHT, OUTPUT);

    // Khởi tạo I2C
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);
    delay(100);
    logInfo("I2C da khoi tao (SDA=21, SCL=22, 100kHz)");

    // Chạy tự động toàn bộ test init
    test0_scanI2C();
    test1_initVL53L0X();
    test2_initMPU6050();
    test4_wallDetection_once();
    test5_mpuYaw_once();

    logHeader("SAN SANG! Nhan '?' de xem menu lenh.");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
    handleSerial();
    doStream();
    if (streamMode) delay(200); // 5Hz khi stream
}
