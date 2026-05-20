# Hướng dẫn Kiểm tra Độc lập VL53L0X Left (Cảm biến khoảng cách bên trái)

Tài liệu này giúp bạn cô lập **duy nhất** cảm biến VL53L0X Left để xác định xem module này có hoạt động ổn định hay không. Hãy đảm bảo MPU6050 vừa thay chạy ổn định, nhưng **trong lúc test cảm biến trái**, nếu có thể hãy rút MPU6050 ra hoặc ít nhất là đảm bảo sơ đồ chân không xung đột.

---

## 1. Sơ đồ Nối Dây (Chỉ dùng ESP32 và VL53L0X Left)

> **Khuyến nghị:** Dùng đoạn dây ngắn nhất có thể, cắm thật chặt.

| Chân VL53L0X | Nối vào ESP32 | Ghi chú |
|:---:|:---:|:---|
| **VIN / VCC** | `3.3V` | Cấp 3.3V an toàn cho cảm biến |
| **GND** | `GND` | Nối chung Mass với ESP32 |
| **SCL** | `GPIO 22` | Xung nhịp I2C |
| **SDA** | `GPIO 21` | Dữ liệu I2C |
| **XSHUT** | `GPIO 17` | Chân bật/tắt cảm biến Left (kéo HIGH để enable) |
| **GPIO1 / INT** | Bỏ trống | Không dùng trong test này |

> **Lưu ý XSHUT:** Chân XSHUT của cảm biến TRÁI phải cắm đúng vào **GPIO 17**. Nếu cắm nhầm, cảm biến sẽ không được đánh thức (wake-up).

---

## 2. Thư viện sử dụng
Trong code test này chúng ta sẽ sử dụng thư viện **Pololu VL53L0X** (thư viện chuẩn đang được dùng trong `platformio.ini`).

## 3. Mã nguồn Code Test VL53L0X Left

File `src/main.cpp` đã được tự động cập nhật code test độc lập cho cảm biến Left. Đoạn code này thực hiện:
- Kéo chân XSHUT của Front (16) và Right (23) xuống LOW để tắt hoàn toàn (tránh nhiễu nếu bạn vẫn đang cắm).
- Kéo chân XSHUT của Left (17) lên HIGH để bật duy nhất cảm biến này.
- Dùng thư viện `Pololu VL53L0X` để khởi tạo.
- In kết quả ra Serial Monitor mỗi 200ms.

---

## 4. Kết quả Mong đợi trên Serial Monitor

Mở **Serial Monitor** ở **115200 baud**. Nếu mọi thứ đúng, bạn sẽ thấy log tương tự như sau:

```
==================================================
>>> TEST DOC LAP VL53L0X LEFT (Trai)
==================================================
[INFO] Dang reset tat ca cac cam bien...
[INFO] Dang bat cam bien LEFT (GPIO 17 -> HIGH)...

==================================================
>>> BUOC 1: Quet I2C Bus
==================================================
[INFO] Dang quet I2C Bus (dia chi 0x01 -> 0x7F)...
[ OK ] Tim thay thiet bi tai 0x29  <-- VL53L0X (Mac dinh)
[INFO] Tong so thiet bi tim thay: 1

==================================================
>>> BUOC 2: Khoi tao VL53L0X LEFT
==================================================
[ OK ] VL53L0X LEFT khoi tao thanh cong!

==================================================
>>> BUOC 3: Doc khoang cach lien tuc (mm)
==================================================
LEFT [mm]: 153
LEFT [mm]: 120
LEFT [mm]: OUT_OF_RANGE
```

> **Thử nghiệm:** Đưa bàn tay lại gần / ra xa phía trước cảm biến trái. Tầm đo hiệu quả là khoảng **30mm đến 1200mm**. Nếu báo `OUT_OF_RANGE` là xa quá hoặc không có vật cản.

---

## 5. Bảng Chẩn đoán Lỗi

| Triệu chứng | Nguyên nhân có thể | Cách xử lý |
|:---|:---|:---|
| `KHONG TIM THAY THIET BI NAO TREN BUS I2C` | Dây SDA/SCL lỏng, sai chân XSHUT (17), thiếu nguồn | Kiểm tra lại dây cắm, cắm chặt các jumper |
| `Khoi tao VL53L0X LEFT THAT BAI` | Có I2C nhưng giao tiếp hỏng | Thử đổi jumper ngắn hơn, kiểm tra nhiễu nguồn |
| Cố định ở 8190 hoặc `OUT_OF_RANGE` liên tục | Cảm biến lỗi quang học hoặc không có vật cản | Đưa tay lại gần hơn để xác nhận |
| `TIMEOUT (Loi giao tiep)` | Đứt kết nối đột ngột khi đang chạy | Cắm chặt dây I2C lại |
