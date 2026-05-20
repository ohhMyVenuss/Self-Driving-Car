#include "robot_common.h"
#include "maze_solver.h"
#include <Wire.h>
#include <MPU6050_light.h>
#include <VL53L0X.h>

// --- CẤU HÌNH PHẦN CỨNG MAZE SOLVER ---
MPU6050 mpu(Wire);

// Khai báo 3 cảm biến khoảng cách (Front, Left, Right) - Pololu VL53L0X
VL53L0X loxFront;
VL53L0X loxLeft;
VL53L0X loxRight;

// ĐỊNH NGHĨA CHÂN XSHUT (THAY ĐỔI THEO SƠ ĐỒ CỦA BẠN)
// Chân XSHUT kéo xuống LOW để tắt cảm biến, kéo lên HIGH để bật và đổi địa chỉ I2C.
#define XSHUT_FRONT 16
#define XSHUT_LEFT  17
#define XSHUT_RIGHT 23

// ĐỊNH NGHĨA KHOẢNG CÁCH NHẬN DIỆN TƯỜNG (mm)
#define WALL_THRESHOLD_MM 150 
#define CELL_MOVE_TIME_MS 800 // THAY ĐỔI: Thời gian (ms) chạy đúng 1 ô mê cung

// --- BIẾN TOÀN CỤC MAZE ---
#define MAZE_SIZE 20 
#define WALL_NORTH 1
#define WALL_EAST  2
#define WALL_SOUTH 4
#define WALL_WEST  8

enum Heading { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 };

Heading currentHeading = NORTH;
int posX = 0;
int posY = 0;
int targetX = MAZE_SIZE - 1;
int targetY = MAZE_SIZE - 1;

int distances[MAZE_SIZE][MAZE_SIZE];
int walls[MAZE_SIZE][MAZE_SIZE];
float targetAngle = 0.0; // Góc mục tiêu cho MPU6050 giữ thẳng xe

// ----------------------------------------------------
// TIỆN ÍCH: SCAN I2C BUS VÀ IN KẾT QUẢ
// ----------------------------------------------------
void scanI2CBus() {
    Serial.println("[I2C] === Bat dau scan I2C bus (SDA=21, SCL=22) ===");
    int found = 0;
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        byte err = Wire.endTransmission();
        if (err == 0) {
            Serial.printf("[I2C]   Thiet bi tim thay tai dia chi: 0x%02X", addr);
            // Gợi ý tên thiết bị theo địa chỉ phổ biến
            if (addr == 0x68 || addr == 0x69) Serial.print("  <-- MPU6050");
            else if (addr == 0x29)             Serial.print("  <-- VL53L0X (dia chi mac dinh)");
            else if (addr == 0x30)             Serial.print("  <-- VL53L0X FRONT");
            else if (addr == 0x31)             Serial.print("  <-- VL53L0X LEFT");
            else if (addr == 0x32)             Serial.print("  <-- VL53L0X RIGHT");
            Serial.println();
            found++;
        } else if (err == 4) {
            Serial.printf("[I2C]   Loi khong xac dinh tai dia chi: 0x%02X (err=%d)\n", addr, err);
        }
    }
    if (found == 0) {
        Serial.println("[I2C] CANH BAO: Khong tim thay thiet bi nao! Kiem tra day noi SDA/SCL va nguon.");
    } else {
        Serial.printf("[I2C] Ket qua: tim thay %d thiet bi.\n", found);
    }
    Serial.println("[I2C] === Ket thuc scan ===");
}

// ----------------------------------------------------
// KHỞI TẠO CẢM BIẾN
// ----------------------------------------------------
void initSensors() {
    Serial.println("[MAZE] ============================================");
    Serial.println("[MAZE] Bat dau khoi tao cam bien (Wire SDA=21, SCL=22)");

    // Kich hoat pull-up noi ESP32 tren SDA/SCL (~47kOhm)
    // Day la giai phap tam thoi - nen them dien tro 4.7kOhm ngoai
    pinMode(21, INPUT_PULLUP); // SDA
    pinMode(22, INPUT_PULLUP); // SCL

    Wire.begin(21, 22);
    Wire.setClock(100000); // 100kHz - chuan, tuong thich toi da voi clone module
    delay(200); // Cho I2C bus va cac module on dinh sau khi cap nguon

    // --- CHAN DOAN: Thu tung toc do I2C va ca 2 dia chi MPU6050 ---
    Serial.println("[MAZE]   [Chan doan] Thu ket noi MPU6050 tai 0x68 va 0x69...");
    bool mpuFound = false;
    uint8_t mpuAddr = 0x68;
    uint32_t speeds[] = {100000, 50000, 400000};
    String speedNames[] = {"100kHz", "50kHz", "400kHz"};

    for (int si = 0; si < 3 && !mpuFound; si++) {
        Wire.setClock(speeds[si]);
        delay(10);
        for (uint8_t addr : {0x68, 0x69}) {
            Wire.beginTransmission(addr);
            Wire.write(0x6B); // PWR_MGMT_1
            Wire.write(0x00); // Clear sleep
            byte s = Wire.endTransmission();
            Serial.printf("[MAZE]     Speed=%-7s Addr=0x%02X Wake status=%d %s\n",
                          speedNames[si].c_str(), addr, s, s == 0 ? "<= OK!" : "");
            if (s == 0) {
                mpuFound = true;
                mpuAddr  = addr;
                Serial.printf("[MAZE]   => MPU tim thay tai 0x%02X, speed=%s\n", addr, speedNames[si].c_str());
                break;
            }
        }
    }
    if (!mpuFound) {
        Serial.println("[MAZE]   => KHONG TIM THAY MPU6050 o moi toc do / dia chi!");
        Serial.println("[MAZE]      Kha nang cao: day SDA/SCL bi hong hoac pull-up qua yeu.");
    }
    // Giu nguyen toc do 400kHz - MPU6050 clone nay can 400kHz
    // (se doi sang 100kHz truoc khi init VL53L0X)

    // --- SCAN I2C TRUOC KHI INIT ---
    Serial.println("[MAZE] [Buoc 0] Scan I2C bus truoc khi init...");
    scanI2CBus();

    // -------------------------------------------------------
    // 1. Khởi tạo MPU6050
    // -------------------------------------------------------
    Serial.println("[MAZE] [Buoc 1] Khoi tao MPU6050 (dia chi 0x68)...");
    byte status = 255;
    for (int attempt = 1; attempt <= 5; attempt++) {
        status = mpu.begin();
        Serial.printf("[MAZE]   Lan thu %d: mpu.begin() = %d", attempt, status);
        if (status == 0) {
            Serial.println(" => THANH CONG");
            break;
        }
        // Giai thich ma loi cu the
        if (status == 1) Serial.println(" => Loi I2C (NACK) - thiet bi khong phan hoi");
        else if (status == 2) Serial.println(" => Loi I2C (NACK on address) - khong thay 0x68");
        else if (status == 3) Serial.println(" => Loi I2C (NACK on data)");
        else if (status == 4) Serial.println(" => Loi I2C khac");
        else                  Serial.printf(" => Loi khong xac dinh (status=%d)\n", status);
        delay(200); // Doi roi thu lai
    }
    if (status != 0) {
        Serial.println("[MAZE]   MPU6050 THAT BAI sau 5 lan thu!");
        Serial.println("[MAZE]   => Kiem tra: day SDA(GPIO21)/SCL(GPIO22), nguon 3.3V, chan AD0 phai o muc LOW");
        Serial.println("[MAZE]   => Tiep tuc khong co MPU6050 (xe se khong giu thang duoc)");
    } else {
        Serial.println("[MAZE]   Dang calib MPU6050, vui long GIU YEN XE 1 giay...");
        delay(1000);
        mpu.calcOffsets();
        Serial.println("[MAZE]   MPU6050 calib xong!");
    }

    // -------------------------------------------------------
    // 2. Khởi tạo 3 cảm biến VL53L0X qua XSHUT (Pololu library)
    // -------------------------------------------------------
    Serial.println("[MAZE] [Buoc 2] Khoi tao 3x VL53L0X qua chan XSHUT...");
    Serial.printf("[MAZE]   XSHUT: FRONT=GPIO%d, LEFT=GPIO%d, RIGHT=GPIO%d\n",
                  XSHUT_FRONT, XSHUT_LEFT, XSHUT_RIGHT);

    pinMode(XSHUT_FRONT, OUTPUT);
    pinMode(XSHUT_LEFT,  OUTPUT);
    pinMode(XSHUT_RIGHT, OUTPUT);

    // Reset tất cả - kéo XSHUT xuống LOW để tắt cảm biến
    digitalWrite(XSHUT_FRONT, LOW);
    digitalWrite(XSHUT_LEFT,  LOW);
    digitalWrite(XSHUT_RIGHT, LOW);
    Serial.println("[MAZE]   Tat het 3 cam bien (XSHUT=LOW)... OK");
    delay(10);

    // --- FRONT: boot lên 0x29, init, đổi sang 0x30 ---
    digitalWrite(XSHUT_FRONT, HIGH);
    delay(10);
    loxFront.setTimeout(500);
    if (!loxFront.init()) {
        Serial.println("[MAZE]   FRONT => THAT BAI! (Kiem tra XSHUT GPIO16, SDA/SCL, VCC 3.3V)");
    } else {
        loxFront.setAddress(0x30);
        Serial.println("[MAZE]   FRONT => OK (dia chi 0x30)");
    }

    // --- LEFT: boot lên 0x29, init, đổi sang 0x31 ---
    digitalWrite(XSHUT_LEFT, HIGH);
    delay(10);
    loxLeft.setTimeout(500);
    if (!loxLeft.init()) {
        Serial.println("[MAZE]   LEFT  => THAT BAI! (Kiem tra XSHUT GPIO17, SDA/SCL, VCC 3.3V)");
    } else {
        loxLeft.setAddress(0x31);
        Serial.println("[MAZE]   LEFT  => OK (dia chi 0x31)");
    }

    // --- RIGHT: boot lên 0x29, init, đổi sang 0x32 ---
    digitalWrite(XSHUT_RIGHT, HIGH);
    delay(10);
    loxRight.setTimeout(500);
    if (!loxRight.init()) {
        Serial.println("[MAZE]   RIGHT => THAT BAI! (Kiem tra XSHUT GPIO23, SDA/SCL, VCC 3.3V)");
    } else {
        loxRight.setAddress(0x32);
        Serial.println("[MAZE]   RIGHT => OK (dia chi 0x32)");
    }

    // --- SCAN I2C SAU KHI INIT ---
    Serial.println("[MAZE] [Buoc 3] Scan I2C bus sau khi init de xac nhan...");
    scanI2CBus();

    Serial.println("[MAZE] ============================================");
    Serial.println("[MAZE] Ket thuc khoi tao cam bien.");
    Serial.println("[MAZE] ============================================");
}

// ----------------------------------------------------
// DI CHUYỂN DÙNG MPU6050 (Không cần Encoder)
// ----------------------------------------------------

// Đi thẳng 1 ô (Dùng MPU6050 bù lệch)
void moveForwardOneCell() {
    Serial.println("[MAZE] Di chuyen 1 o...");
    unsigned long startMs = millis();
    
    float Kp_gyro = 2.0; // Hệ số chỉnh thẳng (Chỉnh lớn nếu xe vẫn bị lạng)
    int base_pwm = 150;
    
    // Chạy tới khi đủ thời gian (hoặc có thể kết hợp VL53L0X Front để phanh gấp)
    while (millis() - startMs < CELL_MOVE_TIME_MS) {
        mpu.update();
        float currentZ = mpu.getAngleZ();
        float error = targetAngle - currentZ;
        
        int correction = (int)(Kp_gyro * error);
        
        // Nếu xe lệch phải (góc âm) -> correction dương -> Tăng L, giảm R
        int leftPWM = base_pwm - correction;
        int rightPWM = base_pwm + correction;
        
        setMotor(leftPWM, rightPWM);
    }
    
    setMotor(0, 0); // Phanh
    
    // Cập nhật hệ tọa độ
    if (currentHeading == NORTH) posY++;
    else if (currentHeading == SOUTH) posY--;
    else if (currentHeading == EAST) posX++;
    else if (currentHeading == WEST) posX--;
}

void turnLeft90() {
    Serial.println("[MAZE] Quay Trai 90 do");
    targetAngle += 90.0; // Góc tăng khi rẽ trái
    
    setMotor(-120, 120);
    while (true) {
        mpu.update();
        if (mpu.getAngleZ() >= targetAngle) break;
    }
    setMotor(0, 0);
    currentHeading = (Heading)((currentHeading + 3) % 4);
}

void turnRight90() {
    Serial.println("[MAZE] Quay Phai 90 do");
    targetAngle -= 90.0; // Góc giảm khi rẽ phải
    
    setMotor(120, -120);
    while (true) {
        mpu.update();
        if (mpu.getAngleZ() <= targetAngle) break;
    }
    setMotor(0, 0);
    currentHeading = (Heading)((currentHeading + 1) % 4);
}

void turnAround180() {
    Serial.println("[MAZE] Quay 180 do");
    targetAngle -= 180.0;
    
    setMotor(120, -120);
    while (true) {
        mpu.update();
        if (mpu.getAngleZ() <= targetAngle) break;
    }
    setMotor(0, 0);
    currentHeading = (Heading)((currentHeading + 2) % 4);
}

// ----------------------------------------------------
// ĐỌC TƯỜNG VL53L0X & THUẬT TOÁN FLOOD FILL
// ----------------------------------------------------

void readWalls() {
    // Pololu VL53L0X: đọc khoảng cách bằng single-shot
    uint16_t mmFront = loxFront.readRangeSingleMillimeters();
    uint16_t mmLeft  = loxLeft.readRangeSingleMillimeters();
    uint16_t mmRight = loxRight.readRangeSingleMillimeters();

    // timeoutOccurred() trả về true nếu đọc bị lỗi/timeout => không có dữ liệu hợp lệ
    bool wallFront = !loxFront.timeoutOccurred() && (mmFront < WALL_THRESHOLD_MM);
    bool wallLeft  = !loxLeft.timeoutOccurred()  && (mmLeft  < WALL_THRESHOLD_MM);
    bool wallRight = !loxRight.timeoutOccurred() && (mmRight < WALL_THRESHOLD_MM);
    
    int currentWallData = 0;
    
    if (currentHeading == NORTH) {
        if (wallFront) { currentWallData |= WALL_NORTH; if (posY + 1 < MAZE_SIZE) walls[posY + 1][posX] |= WALL_SOUTH; }
        if (wallRight) { currentWallData |= WALL_EAST; if (posX + 1 < MAZE_SIZE) walls[posY][posX + 1] |= WALL_WEST; }
        if (wallLeft)  { currentWallData |= WALL_WEST; if (posX - 1 >= 0) walls[posY][posX - 1] |= WALL_EAST; }
    } else if (currentHeading == EAST) {
        if (wallFront) { currentWallData |= WALL_EAST; if (posX + 1 < MAZE_SIZE) walls[posY][posX + 1] |= WALL_WEST; }
        if (wallRight) { currentWallData |= WALL_SOUTH; if (posY - 1 >= 0) walls[posY - 1][posX] |= WALL_NORTH; }
        if (wallLeft)  { currentWallData |= WALL_NORTH; if (posY + 1 < MAZE_SIZE) walls[posY + 1][posX] |= WALL_SOUTH; }
    } else if (currentHeading == SOUTH) {
        if (wallFront) { currentWallData |= WALL_SOUTH; if (posY - 1 >= 0) walls[posY - 1][posX] |= WALL_NORTH; }
        if (wallRight) { currentWallData |= WALL_WEST; if (posX - 1 >= 0) walls[posY][posX - 1] |= WALL_EAST; }
        if (wallLeft)  { currentWallData |= WALL_EAST; if (posX + 1 < MAZE_SIZE) walls[posY][posX + 1] |= WALL_WEST; }
    } else if (currentHeading == WEST) {
        if (wallFront) { currentWallData |= WALL_WEST; if (posX - 1 >= 0) walls[posY][posX - 1] |= WALL_EAST; }
        if (wallRight) { currentWallData |= WALL_NORTH; if (posY + 1 < MAZE_SIZE) walls[posY + 1][posX] |= WALL_SOUTH; }
        if (wallLeft)  { currentWallData |= WALL_SOUTH; if (posY - 1 >= 0) walls[posY - 1][posX] |= WALL_NORTH; }
    }
    
    walls[posY][posX] |= currentWallData; 
}

void floodFillUpdate() {
    for (int y = 0; y < MAZE_SIZE; y++) {
        for (int x = 0; x < MAZE_SIZE; x++) {
            distances[y][x] = 255;
        }
    }
    distances[targetY][targetX] = 0;
    
    struct Point { int x, y; };
    Point queue[MAZE_SIZE * MAZE_SIZE];
    int head = 0, tail = 0;
    queue[tail++] = {targetX, targetY};
    
    while (head < tail) {
        Point p = queue[head++];
        int d = distances[p.y][p.x];
        
        if (p.y + 1 < MAZE_SIZE && !(walls[p.y][p.x] & WALL_NORTH)) {
            if (distances[p.y + 1][p.x] == 255) { distances[p.y + 1][p.x] = d + 1; queue[tail++] = {p.x, p.y + 1}; }
        }
        if (p.y - 1 >= 0 && !(walls[p.y][p.x] & WALL_SOUTH)) {
            if (distances[p.y - 1][p.x] == 255) { distances[p.y - 1][p.x] = d + 1; queue[tail++] = {p.x, p.y - 1}; }
        }
        if (p.x + 1 < MAZE_SIZE && !(walls[p.y][p.x] & WALL_EAST)) {
            if (distances[p.y][p.x + 1] == 255) { distances[p.y][p.x + 1] = d + 1; queue[tail++] = {p.x + 1, p.y}; }
        }
        if (p.x - 1 >= 0 && !(walls[p.y][p.x] & WALL_WEST)) {
            if (distances[p.y][p.x - 1] == 255) { distances[p.y][p.x - 1] = d + 1; queue[tail++] = {p.x - 1, p.y}; }
        }
    }
}

// ----------------------------------------------------
// GIAO DIỆN MAZE CHÍNH
// ----------------------------------------------------

void setupMazeSolver() {
    Serial.println("[MAZE] Khoi tao thuat toan Flood Fill");
    
    initSensors();
    
    for (int y = 0; y < MAZE_SIZE; y++) {
        for (int x = 0; x < MAZE_SIZE; x++) {
            walls[y][x] = 0;
            if (y == 0) walls[y][x] |= WALL_SOUTH;
            if (y == MAZE_SIZE - 1) walls[y][x] |= WALL_NORTH;
            if (x == 0) walls[y][x] |= WALL_WEST;
            if (x == MAZE_SIZE - 1) walls[y][x] |= WALL_EAST;
        }
    }
    floodFillUpdate();
}

void loopMazeSolver() {
    // 1. Đọc tường
    readWalls();
    
    // 2. Tính lại Flood Fill
    floodFillUpdate();
    
    // 3. Nếu đã tới đích (Trường hợp chạm C2 được xử lý ngoài main.cpp, 
    //    nhưng đây là logic dừng riêng của mảng)
    if (posX == targetX && posY == targetY) {
        Serial.println("[MAZE] Da toi dich tren ban do.");
        setMotor(0,0);
        delay(1000);
        return;
    }
    
    // 4. Quyết định hướng đi tiếp theo
    int distFront = 255, distRight = 255, distLeft = 255, distBack = 255;
    
    if (currentHeading == NORTH) {
        if (!(walls[posY][posX] & WALL_NORTH) && posY + 1 < MAZE_SIZE) distFront = distances[posY + 1][posX];
        if (!(walls[posY][posX] & WALL_EAST) && posX + 1 < MAZE_SIZE) distRight = distances[posY][posX + 1];
        if (!(walls[posY][posX] & WALL_WEST) && posX - 1 >= 0) distLeft = distances[posY][posX - 1];
        if (!(walls[posY][posX] & WALL_SOUTH) && posY - 1 >= 0) distBack = distances[posY - 1][posX];
    } else if (currentHeading == EAST) {
        if (!(walls[posY][posX] & WALL_EAST) && posX + 1 < MAZE_SIZE) distFront = distances[posY][posX + 1];
        if (!(walls[posY][posX] & WALL_SOUTH) && posY - 1 >= 0) distRight = distances[posY - 1][posX];
        if (!(walls[posY][posX] & WALL_NORTH) && posY + 1 < MAZE_SIZE) distLeft = distances[posY + 1][posX];
        if (!(walls[posY][posX] & WALL_WEST) && posX - 1 >= 0) distBack = distances[posY][posX - 1];
    } else if (currentHeading == SOUTH) {
        if (!(walls[posY][posX] & WALL_SOUTH) && posY - 1 >= 0) distFront = distances[posY - 1][posX];
        if (!(walls[posY][posX] & WALL_WEST) && posX - 1 >= 0) distRight = distances[posY][posX - 1];
        if (!(walls[posY][posX] & WALL_EAST) && posX + 1 < MAZE_SIZE) distLeft = distances[posY][posX + 1];
        if (!(walls[posY][posX] & WALL_NORTH) && posY + 1 < MAZE_SIZE) distBack = distances[posY + 1][posX];
    } else if (currentHeading == WEST) {
        if (!(walls[posY][posX] & WALL_WEST) && posX - 1 >= 0) distFront = distances[posY][posX - 1];
        if (!(walls[posY][posX] & WALL_NORTH) && posY + 1 < MAZE_SIZE) distRight = distances[posY + 1][posX];
        if (!(walls[posY][posX] & WALL_SOUTH) && posY - 1 >= 0) distLeft = distances[posY - 1][posX];
        if (!(walls[posY][posX] & WALL_EAST) && posX + 1 < MAZE_SIZE) distBack = distances[posY][posX + 1];
    }
    
    int minDist = 255;
    if (distFront < minDist) minDist = distFront;
    if (distRight < minDist) minDist = distRight;
    if (distLeft < minDist) minDist = distLeft;
    if (distBack < minDist) minDist = distBack;
    
    // Di chuyển tới ô có khoảng cách nhỏ nhất
    if (minDist == distFront && distFront != 255) {
        moveForwardOneCell();
    } else if (minDist == distRight && distRight != 255) {
        turnRight90();
        moveForwardOneCell();
    } else if (minDist == distLeft && distLeft != 255) {
        turnLeft90();
        moveForwardOneCell();
    } else { 
        turnAround180();
        moveForwardOneCell();
    }
    
    delay(300); // Dừng tĩnh một chút trước ô tiếp theo để mpu và cảm biến ToF ổn định
}
