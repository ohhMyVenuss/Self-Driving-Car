# Hướng dẫn Kiểm tra Từng Cảm biến I2C (Khi KHÔNG CÓ điện trở ngoài)

Nếu bạn chưa có sẵn điện trở 4.7kΩ, chúng ta sẽ bắt buộc phải dùng lệnh bật điện trở kéo lên nội bộ của ESP32 (`INPUT_PULLUP`) và hạ tốc độ I2C xuống mức thấp để duy trì sự ổn định. 

> **Lưu ý Quan Trọng:** Vì không có trợ lực điện trở ngoài, tín hiệu sẽ rất nhạy cảm. Bạn **PHẢI** sử dụng đoạn dây cắm (SDA, SCL) càng ngắn càng tốt và ấn mạch vào testboard thật chặt.

---

## 1. Sơ đồ Nối Dây (Chung trục I2C)

Tất cả các chân `SDA` đều nối chung vào `GPIO 21` và `SCL` đều nối chung vào `GPIO 22`.

### ESP32 <-> MPU6050 (Cảm biến góc)
| MPU6050 | SDA | SCL | VCC  | GND  |
|---------|-----|-----|------|------|
|         | 21  | 22  | 3.3V | GND  |

### ESP32 <-> Cảm biến khoảng cách VL53L0X (3 cái)
| Cảm biến VL53L0X | SDA | SCL | XSHUT GPIO | VCC  | GND  |
|------------------|-----|-----|------------|------|------|
| Front            | 21  | 22  | 16         | 3.3V | GND  |
| Left             | 21  | 22  | 17         | 3.3V | GND  |
| Right            | 21  | 22  | 23         | 3.3V | GND  |

---

## 2. Mã nguồn Code Test (Phần mềm hỗ trợ tự sửa lỗi)

Đoạn code này đã được mình cấu hình đặc biệt để **tự bật Pull-up nội bộ (`INPUT_PULLUP`)** và ép tốc độ hoạt động xuống cực thấp (50kHz) để bù đắp cho sự thiếu hụt linh kiện phần cứng. Log in ra cũng được chia theo từng giai đoạn 1, 2, 3, 4 rất rõ ràng.

Bạn copy toàn bộ mã này, dán đè vào file `src/main.cpp` (lưu file cũ lại) và nạp thử nhé:

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <Adafruit_VL53L0X.h>

// ==========================================
// ĐỊNH NGHĨA CHÂN MẠCH
// ==========================================
#define SDA_PIN 21
#define SCL_PIN 22

#define XSHUT_FRONT 16
#define XSHUT_LEFT  17
#define XSHUT_RIGHT 23

// Khai báo Object
MPU6050 mpu(Wire);
Adafruit_VL53L0X loxFront = Adafruit_VL53L0X();
Adafruit_VL53L0X loxLeft = Adafruit_VL53L0X();
Adafruit_VL53L0X loxRight = Adafruit_VL53L0X();

void printHeader(String title) {
    Serial.println("\n==================================================");
    Serial.println(">>> " + title);
    Serial.println("==================================================");
}

void setup() {
    Serial.begin(115200);
    delay(2000); // Chờ mở Serial Monitor
    
    printHeader("BAT DAU CHUONG TRINH TEST I2C (KHONG DUNG TRO NGOAI)");

    // 0. BẬT PULL-UP NỘI BỘ ESP32 (VÌ KHÔNG CÓ TRỞ NGOÀI)
    pinMode(SDA_PIN, INPUT_PULLUP);
    pinMode(SCL_PIN, INPUT_PULLUP);
    Serial.println("[INFO] Da bat Pull-up noi bo tren SDA (21) va SCL (22).");

    // 1. ÉP TẤT CẢ VL53L0X NGỦ ĐÔNG BẰNG PHẦN CỨNG
    pinMode(XSHUT_FRONT, OUTPUT);
    pinMode(XSHUT_LEFT, OUTPUT);
    pinMode(XSHUT_RIGHT, OUTPUT);
    digitalWrite(XSHUT_FRONT, LOW);
    digitalWrite(XSHUT_LEFT, LOW);
    digitalWrite(XSHUT_RIGHT, LOW);
    Serial.println("[INFO] Da ep 3 cam bien VL53L0X vao trang thai ngu dong.");
    delay(200); // Chờ điện áp ổn định lại

    // 2. KHỞI TẠO ĐƯỜNG TRUYỀN I2C CHẬM, AN TOÀN
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(50000); // 50kHz cực chậm và ổn định
    Wire.setTimeout(1000); // Timeout 1s tránh treo Error -1
    Serial.println("[INFO] Bus I2C khoi tao thanh cong o 50kHz.");
    delay(500);

    // ----------------------------------------------------
    // BƯỚC 1: TEST MPU6050 (Lúc này 3 con VL53L0X đang ngủ)
    // ----------------------------------------------------
    printHeader("BUOC 1: Kiem tra MPU6050 (Dia chi 0x68)");
    byte mpuStatus = mpu.begin();
    if (mpuStatus == 0) {
        Serial.println("[ OK ] MPU6050 ket noi thanh cong!");
        Serial.println("[INFO] Dang Calibrate MPU6050... (Vui long giu yen mach)");
        mpu.calcOffsets();
        Serial.println("[ OK ] Calibrate MPU6050 xong!");
    } else {
        Serial.print("[FAIL] MPU6050 loi ket noi! Ma loi mpu.begin() = ");
        Serial.println(mpuStatus);
        Serial.println("       -> Kiem tra day noi MPU6050. Rut ra cam lai that chat!");
    }
    delay(1000);

    // ----------------------------------------------------
    // BƯỚC 2: TEST VL53L0X FRONT
    // ----------------------------------------------------
    printHeader("BUOC 2: Kiem tra VL53L0X FRONT (Dia chi moi 0x30)");
    digitalWrite(XSHUT_FRONT, HIGH); // Chỉ đánh thức Front
    delay(100); // Chờ cảm biến thức dậy lâu hơn vì không có trở kéo
    if (loxFront.begin(0x30)) {
        Serial.println("[ OK ] VL53L0X FRONT ket noi thanh cong o dia chi 0x30!");
    } else {
        Serial.println("[FAIL] VL53L0X FRONT loi ket noi!");
        Serial.println("       -> Kiem tra day XSHUT chan 16 hoac day I2C/Nguon.");
    }
    digitalWrite(XSHUT_FRONT, LOW); // Đưa Front về ngủ đông lại để test con tiếp theo
    delay(1000);

    // ----------------------------------------------------
    // BƯỚC 3: TEST VL53L0X LEFT
    // ----------------------------------------------------
    printHeader("BUOC 3: Kiem tra VL53L0X LEFT (Dia chi moi 0x31)");
    digitalWrite(XSHUT_LEFT, HIGH); // Chỉ đánh thức Left
    delay(100);
    if (loxLeft.begin(0x31)) {
        Serial.println("[ OK ] VL53L0X LEFT ket noi thanh cong o dia chi 0x31!");
    } else {
        Serial.println("[FAIL] VL53L0X LEFT loi ket noi!");
        Serial.println("       -> Kiem tra day XSHUT chan 17 hoac day I2C/Nguon.");
    }
    digitalWrite(XSHUT_LEFT, LOW); // Đưa Left về ngủ đông lại
    delay(1000);

    // ----------------------------------------------------
    // BƯỚC 4: TEST VL53L0X RIGHT
    // ----------------------------------------------------
    printHeader("BUOC 4: Kiem tra VL53L0X RIGHT (Dia chi moi 0x32)");
    digitalWrite(XSHUT_RIGHT, HIGH); // Chỉ đánh thức Right
    delay(100);
    if (loxRight.begin(0x32)) {
        Serial.println("[ OK ] VL53L0X RIGHT ket noi thanh cong o dia chi 0x32!");
    } else {
        Serial.println("[FAIL] VL53L0X RIGHT loi ket noi!");
        Serial.println("       -> Kiem tra day XSHUT chan 23 hoac day I2C/Nguon.");
    }
    digitalWrite(XSHUT_RIGHT, LOW); // Đưa Right về ngủ đông lại
    
    // ----------------------------------------------------
    // KẾT THÚC
    // ----------------------------------------------------
    printHeader("HOAN THANH TEST!");
    Serial.println("[INFO] Neu co thiet bi nao [FAIL], hay dung day ngan hon va rut ra cam lai that chat.");
}

void loop() {
    // Không làm gì thêm trong loop
}
```
