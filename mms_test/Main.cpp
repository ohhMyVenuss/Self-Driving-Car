#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>   // Pololu VL53L0X library

// ============================================================
//  CẤU HÌNH PHẦN CỨNG
// ============================================================
#define SDA_PIN      21
#define SCL_PIN      22
#define XSHUT_FRONT  16   // Chân XSHUT của cảm biến Front

// Địa chỉ I2C mặc định của VL53L0X khi mới bật nguồn
#define VL53L0X_DEFAULT_ADDR  0x29

VL53L0X loxFront;   // Pololu API: không cần truyền địa chỉ vào constructor

// ============================================================
//  HÀM HỖ TRỢ
// ============================================================
void printHeader(String title) {
    Serial.println("\n==================================================");
    Serial.println(">>> " + title);
    Serial.println("==================================================");
}

void scanI2C() {
    Serial.println("[INFO] Dang quet I2C Bus...");
    byte error, address;
    int nDevices = 0;

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("[ OK ] Tim thay thiet bi tai dia chi 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            if (address == VL53L0X_DEFAULT_ADDR) {
                Serial.println("  <-- VL53L0X (dia chi mac dinh)");
            } else {
                Serial.println();
            }
            nDevices++;
        } else if (error == 4) {
            Serial.print("[FAIL] Loi chua xac dinh tai dia chi 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
        }
    }

    if (nDevices == 0) {
        Serial.println("[FAIL] KHONG TIM THAY THIET BI I2C NAO!");
        Serial.println("       -> Kiem tra lai:");
        Serial.println("          1. Day cam SDA/SCL co bi long khong?");
        Serial.println("          2. Chan XSHUT (GPIO 16) da duoc noi vao cam bien chua?");
        Serial.println("          3. Cam bien co duoc cap nguon 3.3V khong?");
    }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(2000);

    printHeader("BAT DAU CHUONG TRINH TEST VL53L0X FRONT DOC LAP");

    // --- BUOC 0: Tat cam bien truoc de dam bao trang thai sach ---
    pinMode(XSHUT_FRONT, OUTPUT);
    digitalWrite(XSHUT_FRONT, LOW);
    Serial.println("[INFO] Da keo XSHUT_FRONT xuong LOW (reset cam bien).");
    delay(20);

    // --- BUOC 1: Khoi tao I2C truoc ---
    Wire.begin(SDA_PIN, SCL_PIN);

    // --- BUOC 2: Bat Pull-up noi bo SAU khi Wire.begin de tranh bi ghi de ---
    pinMode(SDA_PIN, INPUT_PULLUP);
    pinMode(SCL_PIN, INPUT_PULLUP);
    Serial.println("[INFO] Da bat Pull-up noi bo (INPUT_PULLUP) cho I2C.");

    Wire.setClock(50000);   // 50 kHz - an toan, chong nhieu
    Wire.setTimeout(1000);
    delay(100);

    // --- BUOC 3: Bat cam bien bang XSHUT ---
    Serial.println("[INFO] Dang kich hoat cam bien XSHUT_FRONT -> HIGH...");
    digitalWrite(XSHUT_FRONT, HIGH);
    delay(20);  // Cho cam bien boot xong

    // --- BUOC 4: Quet I2C ---
    printHeader("BUOC 1: Quet I2C de tim VL53L0X Front");
    scanI2C();

    // --- BUOC 5: Khoi tao cam bien (Pololu API) ---
    printHeader("BUOC 2: Khoi tao VL53L0X Front (Dia chi 0x29)");

    loxFront.setBus(&Wire);
    loxFront.setAddress(VL53L0X_DEFAULT_ADDR);
    loxFront.setTimeout(500);

    if (!loxFront.init()) {
        Serial.println("[FAIL] Khoi tao VL53L0X FRONT that bai!");
        Serial.println("       Goi y xu ly:");
        Serial.println("       1. Kiem tra lai day SDA/SCL va nguon 3.3V.");
        Serial.println("       2. Dam bao XSHUT da duoc noi dung vao GPIO 16.");
        Serial.println("       3. Thu rut day cam bien ra roi cam lai that chat.");
        Serial.println("       4. Thu thay doan day ngan hon.");
        while (1) delay(100); // Treo chuong trinh, doc Serial de xem loi
    }

    // Bat che do do lien tuc (chu ky 50ms)
    loxFront.startContinuous(50);

    Serial.println("[ OK ] VL53L0X FRONT khoi tao thanh cong tai 0x29!");
    Serial.println("[INFO] Dang bat dau do khoang cach... (Don vi: mm)");

    printHeader("DU LIEU KHOANG CACH PHIA TRUOC (mm)");
}

// ============================================================
//  LOOP - Doc va in khoang cach lien tuc
// ============================================================
void loop() {
    uint16_t range = loxFront.readRangeContinuousMillimeters();

    static unsigned long timer = 0;
    if ((millis() - timer) > 200) { // In ra moi 0.2 giay
        Serial.print("FRONT [mm]: ");

        if (loxFront.timeoutOccurred()) {
            Serial.println("TIMEOUT - Kiem tra lai day cam bien!");
        } else if (range >= 8190) {
            // Gia tri >= 8190 nghia la "Out of range" voi Pololu lib
            Serial.println("OUT_OF_RANGE  (> ~1200mm hoac co vat can che laser)");
        } else {
            Serial.println(range);
        }

        timer = millis();
    }
}