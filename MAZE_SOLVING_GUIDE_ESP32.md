# 🚀 Hướng Dẫn Toàn Tập: Chế Tạo & Vận Hành Xe Dò Mê Cung (ESP32)

Tài liệu này là hướng dẫn "cầm tay chỉ việc" từ A đến Z để lắp ráp phần cứng, kết nối điện, và vận hành xe robot giải mê cung tự động một cách trơn tru nhất.

---

## 1. ⚡ Quy Trình Nguồn Điện - BƯỚC QUAN TRỌNG NHẤT
*Rất nhiều board mạch và cảm biến đã bị cháy do bỏ qua bước này.*

1. **Không cắm** ESP32 và DRV8833 vào vội.
2. Nối **Pin Li-ion 7.4V** vào ngõ vào (`IN+` và `IN-`) của mạch giảm áp **LM2596**.
3. Dùng đồng hồ vạn năng (VOM) đo ở ngõ ra (`OUT+` và `OUT-`). Dùng tua vít xoay biến trở trên mạch LM2596 cho đến khi đồng hồ chỉ đúng **5.0V**.
4. Lúc này, bạn mới dùng nguồn 5.0V vừa chỉnh xong để cấp cho:
   - Chân `5V` (L20) của ESP32 Expansion Board.
   - Chân `VCC` của mạch công suất động cơ DRV8833.
5. **Nguyên tắc GND:** Tất cả các linh kiện (Pin, LM2596, ESP32, DRV8833, Cảm biến) ĐỀU PHẢI NỐI CHUNG GND VỚI NHAU. Nếu thiếu GND chung, tín hiệu sẽ bị nhiễu và không hoạt động.

---

## 2. 🔌 Sơ Đồ Nối Dây Chi Tiết (Chế Độ Mê Cung)

Chế độ mê cung không dùng cảm biến quang trở (QTR-8A) mà dùng hệ thống **Laser ToF (VL53L0X)** và **Cảm biến góc (MPU6050)**.

### A. Bus I2C & Cảm Biến
Cả 4 cảm biến (1 con MPU6050 và 3 con VL53L0X) đều giao tiếp chung qua chuẩn I2C.
* **Nguồn cấp:** Cấp nguồn **3.3V** (tuyệt đối không dùng 5V) từ chân 3V3 của ESP32 cho tất cả 4 cảm biến.
* **Tín hiệu I2C:**
  - Nối toàn bộ chân **SDA** của 4 cảm biến lại với nhau, rồi cắm vào chân **GPIO 21** trên ESP32.
  - Nối toàn bộ chân **SCL** của 4 cảm biến lại với nhau, rồi cắm vào chân **GPIO 22** trên ESP32.

### B. Địa chỉ cho 3 con VL53L0X (Chân XSHUT)
Vì 3 cảm biến giống hệt nhau, ta phải dùng chân `XSHUT` để ESP32 bật/tắt từng con và đổi địa chỉ I2C lúc khởi động.
- Cảm biến **Trước (FRONT)**: XSHUT ──► **GPIO 16**
- Cảm biến **Trái (LEFT)**: XSHUT ──► **GPIO 17**
- Cảm biến **Phải (RIGHT)**: XSHUT ──► **GPIO 23**

### C. Mạch Công Suất DRV8833 & Động Cơ (N20)

**1. Nối tín hiệu giữa DRV8833 và ESP32:**
- `VCC`: Cấp nguồn 5V từ LM2596.
- `GND`: Nối chung với GND toàn mạch.
- `AIN1` ──► **GPIO 12** (Điều khiển Motor Trái)
- `AIN2` ──► **GPIO 14** (Điều khiển Motor Trái)
- `BIN1` ──► **GPIO 18** (Điều khiển Motor Phải)
- `BIN2` ──► **GPIO 19** (Điều khiển Motor Phải)
*Mẹo: Nếu cấp nguồn mà bánh không quay dù code đã chạy, hãy gắn thêm 1 sợi dây nối chân `SLP` của DRV8833 lên nguồn 3.3V.*

**2. Nối 2 dây từ Động Cơ N20 vào DRV8833:**
- Lấy 2 sợi dây của **Động cơ bên TRÁI** gắn vào 2 cọc **AOUT1** và **AOUT2** trên mạch DRV8833.
- Lấy 2 sợi dây của **Động cơ bên PHẢI** gắn vào 2 cọc **BOUT1** và **BOUT2** trên mạch DRV8833.
> 💡 **Bí kíp:** Motor N20 không phân biệt chiều âm dương cứng ngắc! Cứ cắm đại 2 dây vào AOUT/BOUT. Lúc cho xe chạy thử trên đất, nếu thấy xe thay vì tiến lên lại lùi xuống, hoặc rẽ trái thành rẽ phải, bạn **chỉ cần rút 2 sợi dây của motor bị ngược ra, đảo vị trí cho nhau** rồi cắm lại là xe sẽ chạy đúng hướng!

### D. Nút Khởi Động Mê Cung (C2)
- 1 chân của nút bấm nối vào **GPIO 15**.
- Chân còn lại nối vào **GND**.

---

## 3. 📐 Vị Trí Lắp Đặt Khung Xe (Cơ Khí)

1. **MPU6050 (Rất Quan Trọng):** Phải gắn **CỐ ĐỊNH, DÍNH CHẶT** vào khung xe (dùng keo nến hoặc ốc vít). Vị trí lý tưởng là nằm ở tâm của 2 trục bánh xe. Nếu MPU6050 bị lỏng lẻo hoặc rung lắc khi xe chạy, góc xoay của xe sẽ bị sai hoàn toàn. Mạch phải nằm ngang song song với mặt đất.
2. **Cảm Biến VL53L0X:**
   - FRONT: Chỉa thẳng ra phía trước đầu xe, không để vật cản che khuất.
   - LEFT / RIGHT: Gắn ở 2 bên thành xe, hướng chính xác góc 90 độ ra 2 bên thành mê cung. Đảm bảo tia laser không chiếu nhầm vào bánh xe.
3. Bóc lớp màng ni-lông bảo vệ trên con chip của 3 cảm biến VL53L0X để đo chính xác nhất.

---

## 4. 🚀 Hướng Dẫn Chạy & Khắc Phục Sự Cố (Troubleshooting)

### Bước 1: Khởi động tĩnh
Khi bật nguồn công tắc, hãy đặt xe **ngay ngắn ở điểm xuất phát và hoàn toàn không chạm vào xe**.
*Lý do:* Trong vài giây đầu tiên, ESP32 sẽ nạp code để reset địa chỉ VL53L0X và quan trọng nhất là **Calibrate (hiệu chuẩn) MPU6050**. Nếu bạn cầm trên tay hoặc rung lắc xe lúc này, MPU sẽ lấy góc sai, dẫn đến xe xoay không đúng 90 độ.

### Bước 2: Kích hoạt
Bấm nút C2 (GPIO 15). Xe sẽ bắt đầu lao đi và chạy thuật toán Flood Fill.

### Bước 3: Theo dõi & Hiệu chỉnh
Khi xe chạy trong mê cung, sẽ có những tình huống sau và cách để bạn khắc phục:

**1. Xe bám sát tường bị đi lạng lách (Oscillation):**
- Do thuật toán PD bù sai số 2 bên hông đang quá mạnh. Bạn cần mở code, giảm biến `Kp` hoặc tăng biến `Kd` trong phần hàm bám tường của mê cung.

**2. Xe không nhận diện được ngã rẽ:**
- Có thể ngưỡng phát hiện tường (Wall Threshold) đang sai. Thông thường mê cung tiêu chuẩn có bề rộng 1 ô là 18-20cm, hãy đo khoảng cách thực tế từ cảm biến ra giữa bức tường và set ngưỡng phát hiện tường `WALL_DISTANCE_THRESHOLD` trong code cho phù hợp (thường khoảng `100mm` đến `120mm`).

**3. Xe quay 90 độ nhưng quay quá trớn hoặc chưa tới:**
- Đây là lỗi thường gặp nhất. Hãy vệ sinh bánh xe. Nếu xe trượt, bánh xe đang quay nhưng xe không xoay thì MPU6050 sẽ cố bù liên tục gây quá lố.
- Thử giảm tốc độ quay (Turn Speed) trong code xuống để xe xoay từ từ, MPU6050 sẽ đọc góc mượt mà hơn.
- Nếu xe bẻ góc ngược (Ví dụ rẽ trái thành rẽ phải): Hãy đổi chỗ cắm 2 dây của motor bên Trái hoặc bên Phải.

**4. Xe đụng tường trước mặt:**
- Kiểm tra lại cảm biến FRONT. Đảm bảo ngưỡng phát hiện phía trước `FRONT_STOP_DISTANCE` phải lớn hơn độ dài của mũi xe tính từ cảm biến, để xe dừng lại kịp thời ở chính giữa ô mê cung trước khi quyết định rẽ. Khoảng cách dừng đẹp nhất thường là `80-100mm` tính từ tâm xe đến tường.
