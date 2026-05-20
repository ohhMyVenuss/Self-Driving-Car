# Hướng dẫn Kiểm tra Độc lập VL53L0X Front (Cảm biến khoảng cách phía trước)

Tài liệu này giúp bạn cô lập **duy nhất** cảm biến VL53L0X Front để xác định xem module này có còn hoạt động hay không. Hãy tháo hoàn toàn dây của các cảm biến VL53L0X Left/Right và MPU6050 ra khỏi bus I2C để tránh bị nhiễu chéo trong lúc test.

---

## 1. Sơ đồ Nối Dây (Chỉ dùng ESP32 và VL53L0X Front)

> **Khuyến nghị:** Dùng đoạn dây ngắn nhất có thể, cắm thật chặt. **Không** cắm thêm bất kỳ thiết bị I2C nào khác vào bus trong lúc test.

| Chân VL53L0X | Nối vào ESP32 | Ghi chú |
|:---:|:---:|:---|
| **VIN / VCC** | `3.3V` | Module Adafruit đã có mạch hạ áp, nhưng cấp 3.3V vẫn an toàn nhất |
| **GND** | `GND` | Nối chung Mass với ESP32 |
| **SCL** | `GPIO 22` | Xung nhịp I2C |
| **SDA** | `GPIO 21` | Dữ liệu I2C |
| **XSHUT** | `GPIO 16` | Chân bật/tắt cảm biến (kéo HIGH để enable) |
| **GPIO1 / INT** | Bỏ trống | Chân ngắt, không dùng trong test này |

> **Lưu ý XSHUT:** Nếu chân XSHUT bị để hở (floating) hoặc bị kéo xuống LOW, cảm biến sẽ không hoạt động dù kết nối I2C hoàn toàn đúng. Code dưới đây sẽ điều khiển chân này tường minh.

---

## 2. Mã nguồn Code Test & Debug Log VL53L0X Front

Đoạn code này sẽ:
- Tự bật điện trở kéo lên bên trong ESP32 (`INPUT_PULLUP`) cho bus I2C.
- Kéo chân `XSHUT` lên `HIGH` để bật cảm biến Front.
- Quét I2C Bus xem có thấy VL53L0X ở địa chỉ mặc định `0x29` không.
- Chạy hàm `begin()` để khởi tạo cảm biến.
- Liên tục in ra khoảng cách đo được (mm) để bạn đưa tay lại gần/xa kiểm tra.

Hãy copy đoạn code dưới đây, dán đè vào file `src/main.cpp` rồi nạp thử xuống ESP32:

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// ============================================================
//  CẤU HÌNH PHẦN CỨNG
// ============================================================
#define SDA_PIN      21
#define SCL_PIN      22
#define XSHUT_FRONT  16   // Chân XSHUT của cảm biến Front

// Địa chỉ I2C mặc định của VL53L0X khi mới bật nguồn
#define VL53L0X_DEFAULT_ADDR  0x29

Adafruit_VL53L0X loxFront = Adafruit_VL53L0X();

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
        Serial.println("          2. Chân XSHUT (GPIO 16) da duoc noi vao cam bien chua?");
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
    // Keo XSHUT xuong LOW de reset cam bien ve trang thai ban dau
    pinMode(XSHUT_FRONT, OUTPUT);
    digitalWrite(XSHUT_FRONT, LOW);
    Serial.println("[INFO] Da keo XSHUT_FRONT xuong LOW (reset cam bien).");
    delay(20);

    // --- BUOC 1: Bat Pull-up I2C noi bo ---
    pinMode(SDA_PIN, INPUT_PULLUP);
    pinMode(SCL_PIN, INPUT_PULLUP);
    Serial.println("[INFO] Da bat Pull-up noi bo (INPUT_PULLUP) cho I2C.");

    // --- BUOC 2: Khoi tao I2C ---
    Wire.begin(SDA_PIN, SCL_PIN);
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

    // --- BUOC 5: Khoi tao cam bien ---
    printHeader("BUOC 2: Khoi tao VL53L0X Front (Dia chi 0x29)");

    // begin() se giu nguyen dia chi mac dinh 0x29 vi chi co 1 cam bien
    if (!loxFront.begin(VL53L0X_DEFAULT_ADDR, false, &Wire)) {
        Serial.println("[FAIL] Khoi tao VL53L0X FRONT that bai!");
        Serial.println("       Goi y xu ly:");
        Serial.println("       1. Kiem tra lai day SDA/SCL va nguon 3.3V.");
        Serial.println("       2. Dam bao XSHUT da duoc noi dung vao GPIO 16.");
        Serial.println("       3. Thu rut day cam bien ra roi cam lai that chat.");
        Serial.println("       4. Thu thay doan day ngan hon.");
        while (1) delay(100); // Treo chuong trinh, doc Serial de xem loi
    }

    Serial.println("[ OK ] VL53L0X FRONT khoi tao thanh cong tai 0x29!");
    Serial.println("[INFO] Dang bat dau do khoang cach... (Don vi: mm)");

    printHeader("DU LIEU KHOANG CACH PHIA TRUOC (mm)");
}

// ============================================================
//  LOOP - Doc va in khoang cach lien tuc
// ============================================================
void loop() {
    VL53L0X_RangingMeasurementData_t measure;
    loxFront.rangingTest(&measure, false); // false = khong in debug raw

    static unsigned long timer = 0;
    if ((millis() - timer) > 200) { // In ra moi 0.2 giay
        Serial.print("FRONT [mm]: ");

        if (measure.RangeStatus != 4) {
            // RangeStatus == 4 nghia la "Out of range" / khong do duoc
            Serial.println(measure.RangeMilliMeter);
        } else {
            Serial.println("OUT_OF_RANGE  (> ~1200mm hoac co vat can che laser)");
        }

        timer = millis();
    }
}
```

---

## 3. Kết quả Mong đợi trên Serial Monitor

Mở **Serial Monitor** ở **115200 baud**. Nếu mọi thứ đúng, bạn sẽ thấy log tương tự như sau:

```
==================================================
>>> BAT DAU CHUONG TRINH TEST VL53L0X FRONT DOC LAP
==================================================
[INFO] Da keo XSHUT_FRONT xuong LOW (reset cam bien).
[INFO] Da bat Pull-up noi bo (INPUT_PULLUP) cho I2C.
[INFO] Dang kich hoat cam bien XSHUT_FRONT -> HIGH...

==================================================
>>> BUOC 1: Quet I2C de tim VL53L0X Front
==================================================
[INFO] Dang quet I2C Bus...
[ OK ] Tim thay thiet bi tai dia chi 0x29  <-- VL53L0X (dia chi mac dinh)

==================================================
>>> BUOC 2: Khoi tao VL53L0X Front (Dia chi 0x29)
==================================================
[ OK ] VL53L0X FRONT khoi tao thanh cong tai 0x29!
[INFO] Dang bat dau do khoang cach... (Don vi: mm)

==================================================
>>> DU LIEU KHOANG CACH PHIA TRUOC (mm)
==================================================
FRONT [mm]: 312
FRONT [mm]: 287
FRONT [mm]: 145
FRONT [mm]: OUT_OF_RANGE  (> ~1200mm hoac co vat can che laser)
```

> **Thử nghiệm:** Đưa bàn tay lại gần / ra xa trước mặt cảm biến. Số `mm` phải thay đổi theo. Tầm đo hiệu quả: **30mm → 1200mm**.

---

## 4. Bảng Chẩn đoán Lỗi

| Triệu chứng | Nguyên nhân có thể | Cách xử lý |
|:---|:---|:---|
| `KHONG TIM THAY THIET BI I2C NAO` | Dây lỏng, XSHUT chưa HIGH, thiếu nguồn | Kiểm tra lại từng dây, đo điện áp VCC |
| `Khoi tao VL53L0X FRONT that bai` | I2C tìm thấy nhưng lib init lỗi | Thử `Wire.setClock(10000)` (10kHz), hoặc thay dây ngắn hơn |
| Số `mm` bị `OUT_OF_RANGE` liên tục | Không có vật cản, hoặc vật cản quá xa | Đưa tay lại gần ~20cm trước cảm biến để thử |
| Số `mm` nhảy loạn, không ổn định | Nhiễu nguồn hoặc ánh sáng mạnh chiếu thẳng | Thêm tụ lọc 100nF giữa VCC-GND, tránh ánh sáng mặt trời |
| `[FAIL] Loi chua xac dinh tai 0x29` (error = 4) | Lỗi I2C cứng — thường do ngắn mạch | Rút hết dây, kiểm tra ngắn mạch giữa SDA và GND |

---

## 5. Ghi chú về Địa chỉ I2C

Cảm biến VL53L0X **luôn boot lên với địa chỉ mặc định `0x29`** mỗi khi XSHUT được reset về LOW rồi kéo HIGH lại.

Trong firmware chính (`maze_solver`), địa chỉ được đổi sang `0x30` (Front), `0x31` (Left), `0x32` (Right) thông qua quá trình khởi tạo tuần tự dùng XSHUT. Trong bài test độc lập này, chúng ta **giữ nguyên địa chỉ `0x29`** để đơn giản hóa.
