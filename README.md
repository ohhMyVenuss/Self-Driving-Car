README — Sơ đồ chân (Wiring)

Mục đích: mô tả cách các chân GPIO được cắm trên bo mạch ESP32 trong dự án `self-driving-car`.

## 🛠️ Sơ Đồ Nối Mạch (Wiring Diagram)

### 1. ESP32 <-> Cảm biến line (QTR-8)
| Cảm biến QTR | GPIO ESP32 |
|--------------|------------|
| Sensor 0     | 32         |
| Sensor 1     | 33         |
| Sensor 2     | 34         |
| Sensor 3     | 35         |
| Sensor 4     | 27         |
| Sensor 5     | 4          |
| Sensor 6     | 25         |
| Sensor 7     | 26         |
| IR_LED       | 13         |

### 2. ESP32 <-> Driver Motor (DRV8833)
| DRV8833 Input | GPIO ESP32 |
|---------------|------------|
| AIN1          | 12         |
| AIN2          | 14         |
| BIN1          | 18         |
| BIN2          | 19         |

- VCC DRV8833: 5V
- GND DRV8833 nối chung GND ESP32

### 3. ESP32 <-> Cảm biến va chạm (Chạm trạm C1, C2)
| Cảm biến      | GPIO ESP32 |
|---------------|------------|
| C1            | 5          |
| C2            | 15         |

### 4. ESP32 <-> Cảm biến khoảng cách VL53L0X (3 cái, I2C, dùng XSHUT)
| Cảm biến VL53L0X | SDA | SCL | XSHUT GPIO | VCC  | GND  |
|------------------|-----|-----|------------|------|------|
| Front            | 21  | 22  | 16         | 3.3V | GND  |
| Left             | 21  | 22  | 17         | 3.3V | GND  |
| Right            | 21  | 22  | 23         | 3.3V | GND  |

- Lưu ý: SDA/SCL dùng chung bus với MPU6050. XSHUT giúp đổi địa chỉ I2C từng cảm biến.

### 5. ESP32 <-> MPU6050 (Cảm biến góc, I2C)
| MPU6050 | SDA | SCL | VCC  | GND  |
|---------|-----|-----|------|------|
|         | 21  | 22  | 3.3V | GND  |

---

**Tóm tắt sơ đồ tổng thể:**
- ESP32 (30 chân)
- 3x VL53L0X (Front, Left, Right) nối I2C (SDA 21, SCL 22), mỗi cảm biến có XSHUT riêng (16, 17, 23)
- 1x MPU6050 nối I2C (SDA 21, SCL 22)
- 1x DRV8833 nối các chân 12, 14, 18, 19
- 1x QTR-8 nối các chân 32, 33, 34, 35, 27, 4, 25, 26
- 2x cảm biến va chạm (C1: 5, C2: 15)

1) Cảm biến IR (QTR, 8 kênh analog)
- Cảm biến thứ tự từ trái sang phải (index 0..7):
  - Sensor 0 -> GPIO 32
  - Sensor 1 -> GPIO 33
  - Sensor 2 -> GPIO 34
  - Sensor 3 -> GPIO 35
  - Sensor 4 -> GPIO 27
  - Sensor 5 -> GPIO 4
  - Sensor 6 -> GPIO 25
  - Sensor 7 -> GPIO 26
- Lưu ý: code xử lý bằng ADC 12-bit (0..4095) và gọi `analogSetPinAttenuation(..., ADC_11db)`.
- LED IR control: `IR_LED_PIN` = GPIO 13 (đặt `pinMode(13, OUTPUT)` trong code và bật tắt LED IR).

2) Motor driver (DRV8833) chân điều khiển PWM
- LEFT motor (A):
  - AIN1 -> GPIO 12
  - AIN2 -> GPIO 14
- RIGHT motor (B):
  - BIN1 -> GPIO 18
  - BIN2 -> GPIO 19
- Trong code PWM (ESP32 LEDC):
  - CH_AIN1 = 0  (gắn với AIN1)
  - CH_AIN2 = 1  (gắn với AIN2)
  - CH_BIN1 = 2  (gắn với BIN1)
  - CH_BIN2 = 3  (gắn với BIN2)
  - Tần số PWM: `PWM_FREQ` = 20000 Hz
  - Độ phân giải: `PWM_RESOLUTION` = 8 bit (0..255)

3) Ghi chú kết nối phần cứng
- Cấp nguồn cho DRV8833: VCC (thông thường 5V) và GND chung với ESP32.
- Các chân AINx/BINx được nối tới inputs điều khiển trên DRV8833 (không nối trực tiếp motor).
- Đảm bảo nối GND ESP32 và GND nguồn motor chung.

4) Kiểm tra/Chẩn đoán nhanh
- Upload firmware rồi mở Serial Monitor (115200): code in ra dòng dạng:
  RAW=[... ] | NORM=[... ] | pos=... | pid=... | motor(L,R)=(Lval, Rval)
  - Nếu `motor(L,R)` hiển thị `0` cho một bên: kiểm tra chân PWM và đường dây tới DRV8833 của bên đó.
  - Nếu cả hai motor hiển thị giá trị nhưng chỉ một bánh quay: kiểm tra kết nối cơ (dây, hàn), và nguồn motor.
- Test motor đơn giản (nếu cần): ngắt sensor loop và điều khiển tay bằng `setMotor(VAL,0)` để chạy riêng trái, hoặc `setMotor(0,VAL)` để chạy riêng phải.

5) File tham chiếu trong project
- Định nghĩa chân: `src/robot_common.h`
- Mảng chân cảm biến: `src/line_follower.cpp` (line: `const int SENSOR_PINS[SENSOR_COUNT] = {32,33,34,35,27,4,25,26};`)

6) Mẹo khắc phục:
- Nếu chỉ có 1 bánh chạy: đổi dây PWM giữa AIN1/AIN2 và BIN1/BIN2 tạm để kiểm tra xem vấn đề nằm ở ESP32 (pin/PWM) hay DRV8833/motor.
- Kiểm tra điện áp cấp motor khi tải (nhiều khi motor không quay nếu điện áp sụt).

7) 🎯 Thuật toán Dò Line Màu Đen (Black Line Following)

**Logic:**
- Thứ tự cảm biến: Từ trái (0) sang phải (7), trọng số: {0, 1000, 2000, 3000, 4000, 5000, 6000, 7000}.
- **Binary Detection (chỉ 4095 = đen):**
  - Chỉ khi RAW = 4095 (tolerance 4085..4095) → NORM = 1000 (phát hiện line đen)
  - Tất cả các giá trị khác (0..4084) → NORM = 0 (không phải line đen)
  
**Vị trí và Điều khiển:**
- Tính vị trí theo công thức: **Position = Σ(WEIGHTs[i] × NORM[i]) / Σ(NORM[i])**
- Tâm line: Position = 3500
  - Position < 3500: Lệch phải → cần bẻ trái
  - Position > 3500: Lệch trái → cần bẻ phải
  
- **PID Controller:**
  - Error = Position - 3500
  - Correction = Kp×Error + Ki×∫Error + Kd×dError/dt
  - Motor L = baseSpeed + Correction
  - Motor R = baseSpeed - Correction

**Phù hợp cho:**
- Sàn có line/track chính xác ở giá trị 4095, nền là các giá trị khác.

8) 📊 Tinh chỉnh & Chẩn đoán

**Đọc Serial Output (115200 baud):**
```
RAW=[...] | NORM=[...] | pos=3500 | err=0 | pid=0 | motor(L,R)=(100,100)
```

**Các trường hợp thường gặp:**

| Hiện tượng | Nguyên nhân | Giải pháp |
|-----------|-----------|----------|
| Tất cả NORM = 0 trên line đen | Threshold quá cao | Giảm `BLACK_RAW_THRESHOLD` → 2500 hoặc 2000 |
| Xa line thì NORM = 1000 | Cảm biến bị lệch hướng | Chỉnh góc/vị trí cảm biến |
| Position luôn < 3500 (lệch phải) | Cảm biến trái bào hoặc yếu | Kiểm tra cảm biến sensor 0..3 |
| Position luôn > 3500 (lệch trái) | Cảm biến phải nhạy/bao | Kiểm tra cảm biến sensor 4..7 |
| Quay quá mạnh (correction lớn) | Kd/Kp quá cao | Giảm `Kp` hoặc tăng `Kd` |
| Chạy quần quạ (không trơn) | Nhiễu nhiều | Tăng `LOW_PASS_ALPHA` → 0.5 |

**Tuning nhanh:**
1. Upload firmware, để robot đi qua line đen.
2. Xem serial `RAW` khi trên line: phải > 2750.
3. Nếu < 2750, hạ `BLACK_RAW_THRESHOLD` về 2500 rồi upload lại.
4. Xem `pos`, `err`: nếu >= ±500, điều chỉnh `Kp` (0.1..0.3) hoặc `Kd` (0.2..0.5).
5. Nếu 1 bánh không chạy: xem `motor(L,R)` có 0 không → check chân/dây DRV8833.

**File cấu hình chính:**
- `src/line_follower.cpp` (line ~20): `BLACK_RAW_THRESHOLD`, `Kp`, `Ki`, `Kd`, `baseSpeed`
- `src/robot_common.h`: Định nghĩa chân GPIO

9) 🛠️ Khác

Nếu muốn: 
- Thêm chế độ test motor tự động (quay từng bên 3s).
- Hoặc hướng dẫn kiểm tra bằng đa-kim.
