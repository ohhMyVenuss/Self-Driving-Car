# 🔌 Sơ Đồ Nối Dây Chuẩn — ESP32S 30P Expansion Board
> **Xe Scout: Dò Line Đen + Dò Mê Cung**  
> Board: ESP32S 30-Pin | Cập nhật: 25/05/2026

---

## 📌 Pinout ESP32S 30P (Chuẩn DOIT DevKit V1)

```
                        ╔══[USB]══╗
           ┌────────────╨─────────╨────────────┐
           │                                    │
     GND  ◄─┤ L01                      R01 ├─► GND
     3V3  ◄─┤ L02  3V3            3V3  R02 ├─► 3V3    (dùng cho sensor)
      EN  ──┤ L03  EN              23  R03 ├─► GPIO23  → XSHUT RIGHT VL53
   IO36*  ──┤ L04  IO36(VP) → Mắt 1    22  R04 ├─► GPIO22  → SCL (I2C)
   IO39*  ──┤ L05  IO39(VN) → Mắt 2    TX  R05 ├─► GPIO1   (TX - không dùng)
   IO34*  ──┤ L06  IO34     → Mắt 3    RX  R06 ├─► GPIO3   (RX - không dùng)
   IO35*  ──┤ L07  IO35     → Mắt 4    21  R07 ├─► GPIO21  → SDA (I2C)
    IO32  ──┤ L08  IO32     → Mắt 5   GND  R08 ├─► GND
    IO33  ──┤ L09  IO33     → Mắt 6    19  R09 ├─► GPIO19  → DRV8833 BIN2
    IO25  ──┤ L10  IO25     → Mắt 7    18  R10 ├─► GPIO18  → DRV8833 BIN1
    IO26  ──┤ L11  IO26     → Mắt 8     5  R11 ├─► GPIO5   → Nút C1 (Line)
    IO27  ──┤ L12  IO27                17  R12 ├─► GPIO17  → XSHUT LEFT VL53
    IO14  ──┤ L13  IO14                16  R13 ├─► GPIO16  → XSHUT FRONT VL53
    IO12  ──┤ L14  IO12                 4  R14 ├─► GPIO4   (trống)
     GND ──┤ L15  GND              0  R15 ├─► GPIO0   ⚠️ Strapping - không dùng
   IO13  ──┤ L16  IO13             2  R16 ├─► GPIO2   ⚠️ Strapping - không dùng
    D2†  ──┤ L17  D2(IO9)         15  R17 ├─► GPIO15  → Nút C2 (Maze)
    D3†  ──┤ L18  D3(IO10)        D1  R18 ├─► D1(IO8)† (Flash - không dùng)
   CMD†  ──┤ L19  CMD(IO11)       D0  R19 ├─► D0(IO7)† (Flash - không dùng)
      5V ◄─┤ L20  5V             CLK  R20 ├─► CLK(IO6)†(Flash - không dùng)
           │                                    │
           └────────────────────────────────────┘

  * = INPUT ONLY (không xuất được)
  † = Flash pins (không bao giờ dùng)
  ⚠️ = Strapping pin (cẩn thận)
```

---

## ⚡ Sơ Đồ Nguồn (LM2596)

```
  Pin Li-ion 7.4V (2S)
       │
       ├─[+]──► LM2596 IN+
       │        LM2596 OUT+ (chỉnh = 5.0V trước khi cắm!)
       │             │
       │             ├──► ESP32  chân 5V (L20)   → Board tự ra 3.3V nội bộ
       │             └──► DRV8833 VCC             → Cấp cho motor
       │
       └─[−]──► GND chung ──► ESP32 GND + DRV8833 GND + tất cả GND
```

---

## 🔧 Nối Dây Từng Module

### 1. DRV8833 Motor Driver

| DRV8833 | ESP32 GPIO | Chân board | Ghi chú |
|---------|-----------|-----------|---------|
| AIN1 | GPIO **12** | L14 | PWM Motor Trái (tiến) |
| AIN2 | GPIO **14** | L13 | PWM Motor Trái (lùi) |
| BIN1 | GPIO **18** | R10 | PWM Motor Phải (tiến) |
| BIN2 | GPIO **19** | R09 | PWM Motor Phải (lùi) |
| VCC | **5V** | L20 | Từ LM2596 |
| GND | GND | L15 | GND chung |
| AOUT1/2 | Motor N20 Trái | — | Đổi 2 dây nếu quay ngược |
| BOUT1/2 | Motor N20 Phải | — | Đổi 2 dây nếu quay ngược |

> ⚠️ Nếu motor không chạy → nối thêm chân **SLP lên 3V3**

---

### 2. MPU6050 (I2C)

| MPU6050 | ESP32 GPIO | Chân board | Ghi chú |
|---------|-----------|-----------|---------|
| VCC | 3V3 | L02 | **KHÔNG dùng 5V** |
| GND | GND | L15 | GND chung |
| SDA | GPIO **21** | R07 | Bus I2C chung |
| SCL | GPIO **22** | R04 | Bus I2C chung |
| AD0 | GND | L15 | Địa chỉ = 0x68 |
| INT | — | — | Bỏ trống |

---

### 3. VL53L0X × 3 (I2C + XSHUT)

Cả 3 cùng dùng: **SDA=GPIO21 (R07), SCL=GPIO22 (R04), VCC=3V3 (L02), GND**

| Cảm biến | XSHUT GPIO | Chân board | Địa chỉ I2C |
|----------|-----------|-----------|------------|
| **FRONT** | GPIO **16** | R13 | 0x30 |
| **LEFT**  | GPIO **17** | R12 | 0x31 |
| **RIGHT** | GPIO **23** | R03 | 0x32 |

> 💡 Hướng gắn: FRONT nhìn thẳng trước, LEFT nhìn trái, RIGHT nhìn phải

---

### 4. Cảm Biến Dò Line 8 Mắt (Analog)

**Sơ đồ chân trên cảm biến:** `1 3 5 7 GND` và `VCC 2 4 6 8`
Chân 1 = ngoài cùng **trái**, Chân 8 = ngoài cùng **phải** (nhìn từ trên xuống theo hướng xe đi)

| Chân Cảm Biến | ESP32 GPIO | Chân board | Ghi chú |
|--------------|-----------|-----------|---------|
| VCC | 3V3 | L02 | |
| GND | GND | L15 | |
| 1 | GPIO **36 (VP)** | L04 | Cảm biến trái nhất (INPUT ONLY) |
| 2 | GPIO **39 (VN)** | L05 | (INPUT ONLY) |
| 3 | GPIO **34** | L06 | (INPUT ONLY) |
| 4 | GPIO **35** | L07 | (INPUT ONLY) |
| 5 | GPIO **32** | L08 | |
| 6 | GPIO **33** | L09 | |
| 7 | GPIO **25** | L10 | |
| 8 | GPIO **26** | L11 | Cảm biến phải nhất |

---

### 5. Nút Bấm Chọn Chế Độ

| Nút | GPIO | Chân board | Chức năng |
|-----|------|-----------|-----------|
| C1 | GPIO **5**  | R11 | Chế độ **Dò Line** |
| C2 | GPIO **15** | R17 | Chế độ **Dò Mê Cung** |

Đấu: 1 đầu → GPIO, đầu còn lại → GND. Code dùng `INPUT_PULLUP`.

---

## 🗺️ Sơ Đồ Tổng Thể Trên Xe

```
                ┌─ VL53L0X-LEFT   VL53L0X-FRONT   VL53L0X-RIGHT ─┐
                │         (GPIO17)    (GPIO16)    (GPIO23)         │
                │              ↕ I2C SDA=21 SCL=22 ↕              │
                └─────────────────────────────────────────────────┘
                                   PHÍA TRƯỚC

  Motor N20 TRÁI              ESP32S 30P                Motor N20 PHẢI
 [AIN1=12, AIN2=14]         ┌──────────┐             [BIN1=18, BIN2=19]
         │                  │ DRV8833  │                      │
         └──────── AOUT ────┤ MPU6050  ├──── BOUT ───────────┘
                            │ LM2596   │
                            │ (nguồn)  │
                            └──────────┘

                ┌─────── Cảm Biến Line 8 Mắt ──────────┐
                │   1    2    3    4    5    6    7    8│
                │ G36  G39  G34  G35  G32  G33  G25  G26│
                │ (←trái)                    (phải→)    │
                └──────────────────────────────────────┘
                              PHÍA SAU
                         [Bánh mắt trâu]
```

---

## ✅ Checklist Kiểm Tra Trước Khi Bật Nguồn

- [ ] LM2596 OUT đo được đúng **5.0V** (trước khi cắm ESP32)
- [ ] Tất cả GND nối **chung một điểm**
- [ ] MPU6050, VL53L0X×3, QTR-8A dùng **3.3V** (từ chân L02 của board)
- [ ] DRV8833 VCC dùng **5V** (từ LM2596, chân L20 của board)
- [ ] XSHUT: FRONT→GPIO16, LEFT→GPIO17, RIGHT→GPIO23
- [ ] SDA (GPIO21) và SCL (GPIO22) có **điện trở 4.7kΩ lên 3.3V**
- [ ] QTR-8A gắn phía dưới xe, cách sàn **3–5mm**, mặt sensor hướng xuống
- [ ] GPIO 0 và GPIO 2 **không nối gì** (strapping pin)

---

## 🐛 Xử Lý Sự Cố Nhanh

| Hiện tượng | Kiểm tra |
|-----------|---------|
| ESP32 không boot | GPIO12 bị kéo HIGH? → thêm 10kΩ xuống GND |
| Motor không quay | DRV8833 SLP=LOW? → nối SLP lên 3V3 |
| I2C không thấy sensor | Thiếu pull-up 4.7kΩ trên SDA/SCL? |
| VL53L0X FAIL | XSHUT cắm sai GPIO? Cắm nhầm 16/17/23? |
| Cảm biến Line toàn 0 | VCC/GND cắm sai? Cảm biến quá xa sàn (>1cm)? |
| Xe bẻ lái ngược | Đảo 1↔8: nối chân 1→GPIO26, chân 8→GPIO32 |
