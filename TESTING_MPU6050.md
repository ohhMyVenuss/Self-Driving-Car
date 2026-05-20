# Hướng dẫn Kiểm tra Độc lập MPU6050 (Cảm biến góc)

Tài liệu này giúp bạn cô lập duy nhất MPU6050 để test xem module này có còn hoạt động hay không. Hãy tháo hoàn toàn các dây của cảm biến VL53L0X ra khỏi mạch I2C để tránh bị nhiễu chéo trong lúc test.

---

## 1. Sơ đồ Nối Dây (Chỉ dùng ESP32 và MPU6050)

> **Khuyến nghị:** Cắm dây trực tiếp từ ESP32 sang MPU6050, sử dụng đoạn dây ngắn nhất có thể và cắm thật chặt.

| Chân MPU6050 | Nối vào ESP32 | Ghi chú |
|:---:|:---:|:---|
| **VCC** | `3.3V` | Tuyệt đối không cắm vào 5V nếu không có mạch chuyển mức |
| **GND** | `GND` | Nối chung Mass với ESP32 |
| **SCL** | `GPIO 22` | Xung nhịp I2C |
| **SDA** | `GPIO 21` | Dữ liệu I2C |
| **AD0** | Bỏ trống | Mặc định địa chỉ I2C là `0x68` |

---

## 2. Mã nguồn Code Test & Debug Log MPU6050

Đoạn code này sẽ:
- Tự bật điện trở kéo lên bên trong ESP32 (`INPUT_PULLUP`).
- Quét I2C Bus xem có thấy MPU6050 không.
- Chạy hàm `begin()` và `calcOffsets()` để xem MPU6050 có phản hồi data không.
- Liên tục in ra góc Z (Góc rẽ trái/phải) để bạn cầm mạch xoay thử xem cảm biến có chạy đúng không.

Bạn hãy copy đoạn code dưới đây, dán đè vào file `src/main.cpp` rồi nạp thử xuống ESP32:

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <MPU6050_light.h>

#define SDA_PIN 21
#define SCL_PIN 22

MPU6050 mpuTest(Wire);

void printHeader(String title) {
    Serial.println("\n==================================================");
    Serial.println(">>> " + title);
    Serial.println("==================================================");
}

void scanI2C() {
    Serial.println("[INFO] Dang quet I2C Bus...");
    byte error, address;
    int nDevices = 0;

    for(address = 1; address < 127; address++ ) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("[ OK ] Tim thay thiet bi tai dia chi 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            if (address == 0x68 || address == 0x69) {
                Serial.println("  <-- MPU6050");
            } else {
                Serial.println();
            }
            nDevices++;
        }
        else if (error == 4) {
            Serial.print("[FAIL] Loi chua xac dinh tai dia chi 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
        }    
    }
    if (nDevices == 0) {
        Serial.println("[FAIL] KHONG TIM THAY THIET BI I2C NAO!");
        Serial.println("       -> Kiem tra lai day cắm, nguon 3.3V hoac ESP32 chưa có Pull-up.");
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000); 

    printHeader("BAT DAU CHUONG TRINH TEST MPU6050 DOC LAP");

    // 1. Cấu hình Pull-up nội bộ
    pinMode(SDA_PIN, INPUT_PULLUP);
    pinMode(SCL_PIN, INPUT_PULLUP);
    Serial.println("[INFO] Da bat Pull-up noi bo (INPUT_PULLUP) cho I2C.");

    // 2. Khởi tạo I2C tốc độ chậm để chống nhiễu
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(50000); // 50kHz cực an toàn
    Wire.setTimeout(1000);
    delay(500);

    // 3. Scan I2C
    printHeader("BUOC 1: Quet I2C de tim kiem MPU6050");
    scanI2C();

    // 4. Khởi tạo MPU6050
    printHeader("BUOC 2: Khoi tao MPU6050 (Dia chi 0x68)");
    byte status = mpuTest.begin();
    
    if (status == 0) {
        Serial.println("[ OK ] MPU6050 phan hoi thanh cong!");
        Serial.println("[INFO] Dang Calibrate MPU6050... (VUI LONG GIU YEN MACH TRONG 2 GIAY)");
        
        delay(1000);
        mpuTest.calcOffsets();
        
        Serial.println("[ OK ] Calibrate thanh cong! Dang doc du lieu...");
        printHeader("DU LIEU GOC Z (XOAY TRAI/PHAI)");
    } else {
        Serial.print("[FAIL] Khoi tao MPU6050 that bai! Ma loi: ");
        Serial.println(status);
        Serial.println("       Goi y ma loi:");
        Serial.println("       1, 2, 3: Loi truyen nhan I2C (Day long, thieu dien tro)");
        Serial.println("       -> Hay rut day MPU ra cam lai chat hon roi reset mach.");
        while(1) delay(100); // Treo máy
    }
}

void loop() {
    // 5. Đọc và in dữ liệu góc Z liên tục (Nhanh, sạch)
    mpuTest.update();
    
    static unsigned long timer = 0;
    if ((millis() - timer) > 200) { // In ra moi 0.2 giay
        Serial.print("Goc Z hien tai: ");
        Serial.println(mpuTest.getAngleZ());
        timer = millis();
    }
}
```
