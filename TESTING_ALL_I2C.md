# Hướng dẫn Tích hợp 4 Cảm biến I2C

Tài liệu này hướng dẫn cách chạy song song 3 cảm biến VL53L0X và 1 cảm biến MPU6050 trên cùng bus I2C.

## 1. Sơ đồ Nối Dây (Wiring)

**Đường truyền I2C (Cắm song song toàn bộ 4 module vào 2 pin này):**
- **SDA** -> GPIO 21
- **SCL** -> GPIO 22

**Nguồn điện (Cắm song song toàn bộ 4 module):**
- **VCC** -> 3.3V (hoặc 5V tùy vào module bạn đang dùng, khuyến nghị MPU6050 cắm 5V nếu module GY-521, còn VL53L0X cắm 3.3V).
- **GND** -> GND chung.

**Cấp phát địa chỉ động (RẤT QUAN TRỌNG):**
- **XSHUT Front** -> GPIO 16
- **XSHUT Left**  -> GPIO 17
- **XSHUT Right** -> GPIO 23

*(Lưu ý: MPU6050 không cần XSHUT, địa chỉ của nó luôn cố định là 0x68).*

## 2. Cách thức hoạt động
Khi bật nguồn, cả 3 con VL53L0X đều mang địa chỉ mặc định là `0x29`, gây xung đột. Do đó ESP32 sẽ làm các bước sau (code `src/test_all_i2c.cpp` đã được lập trình sẵn việc này):
1. Đẩy 3 chân XSHUT xuống LOW (Tắt cả 3 con).
2. Kéo XSHUT Left lên HIGH -> Giao tiếp với nó ở `0x29` -> Ra lệnh đổi thành `0x31`.
3. Kéo XSHUT Front lên HIGH -> Giao tiếp với nó ở `0x29` -> Ra lệnh đổi thành `0x30`.
4. Kéo XSHUT Right lên HIGH -> Giao tiếp với nó ở `0x29` -> Ra lệnh đổi thành `0x32`.

Kết quả: Trên bus sẽ có 4 thiết bị với 4 địa chỉ riêng biệt: `0x30`, `0x31`, `0x32`, và `0x68`.

## 3. Chạy Test
1. Mở PlatformIO, mạch mặc định đã được set thành `test_all_i2c`. Nhấn **Upload** để nạp code.
2. Mở **Serial Monitor** ở baud `115200`.
3. Trong lúc khởi động MPU6050 báo `Dang Calibrate...`, **TUYỆT ĐỐI GIỮ YÊN XE**, không rung lắc để cảm biến cân bằng.
4. Bạn sẽ thấy log cập nhật liên tục:
```text
L: 150mm | F: MAX  | R: 85mm  | Yaw: 15.2 deg
```
- Đưa tay lại gần từng cảm biến xem số liệu có thay đổi đúng không.
- Xoay thân xe sang trái/phải xem góc Yaw có thay đổi chính xác không.

> **Nếu một giá trị bị đơ / kẹt / timeout:** Chứng tỏ dây nối lỏng, bạn cần kiểm tra lại dây và nhấn RESET trên board mạch.
