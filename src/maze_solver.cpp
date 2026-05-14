#include "robot_common.h"
#include "maze_solver.h"
#include <Wire.h>
#include <MPU6050_light.h>
#include <Adafruit_VL53L0X.h>

// --- CẤU HÌNH PHẦN CỨNG MAZE SOLVER ---
MPU6050 mpu(Wire);

// Khai báo 3 cảm biến khoảng cách (Front, Left, Right)
Adafruit_VL53L0X loxFront = Adafruit_VL53L0X();
Adafruit_VL53L0X loxLeft = Adafruit_VL53L0X();
Adafruit_VL53L0X loxRight = Adafruit_VL53L0X();

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
// KHỞI TẠO CẢM BIẾN
// ----------------------------------------------------
void initSensors() {
    Wire.begin();
    
    // 1. Khởi tạo MPU6050
    Serial.println("[MAZE] Khoi tao MPU6050...");
    byte status = mpu.begin();
    if (status != 0) {
        Serial.println("[MAZE] LOI: Khong tim thay MPU6050");
    } else {
        Serial.println("[MAZE] Dang calib MPU6050. Vui long GIU YEN XE...");
        delay(1000);
        mpu.calcOffsets(); 
        Serial.println("[MAZE] MPU6050 Calib Xong!");
    }
    
    // 2. Khởi tạo 3 cảm biến VL53L0X
    Serial.println("[MAZE] Khoi tao VL53L0X...");
    pinMode(XSHUT_FRONT, OUTPUT);
    pinMode(XSHUT_LEFT, OUTPUT);
    pinMode(XSHUT_RIGHT, OUTPUT);
    
    // Đưa tất cả vào trạng thái Reset (Tắt)
    digitalWrite(XSHUT_FRONT, LOW);
    digitalWrite(XSHUT_LEFT, LOW);
    digitalWrite(XSHUT_RIGHT, LOW);
    delay(10);
    
    // Bật Front và cấp địa chỉ 0x30
    digitalWrite(XSHUT_FRONT, HIGH);
    delay(10);
    if (!loxFront.begin(0x30)) Serial.println(F("[MAZE] LOI: VL53L0X FRONT khong ket noi"));
    
    // Bật Left và cấp địa chỉ 0x31
    digitalWrite(XSHUT_LEFT, HIGH);
    delay(10);
    if (!loxLeft.begin(0x31)) Serial.println(F("[MAZE] LOI: VL53L0X LEFT khong ket noi"));
    
    // Bật Right và cấp địa chỉ 0x32
    digitalWrite(XSHUT_RIGHT, HIGH);
    delay(10);
    if (!loxRight.begin(0x32)) Serial.println(F("[MAZE] LOI: VL53L0X RIGHT khong ket noi"));
    
    Serial.println("[MAZE] Khoi tao VL53L0X OK.");
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
    VL53L0X_RangingMeasurementData_t measureFront, measureLeft, measureRight;
    
    loxFront.rangingTest(&measureFront, false);
    loxLeft.rangingTest(&measureLeft, false);
    loxRight.rangingTest(&measureRight, false);
    
    bool wallFront = false, wallLeft = false, wallRight = false;
    
    // Nếu RangeStatus != 4 và khoảng cách nhỏ hơn giới hạn => có tường
    if (measureFront.RangeStatus != 4 && measureFront.RangeMilliMeter < WALL_THRESHOLD_MM) wallFront = true;
    if (measureLeft.RangeStatus != 4 && measureLeft.RangeMilliMeter < WALL_THRESHOLD_MM) wallLeft = true;
    if (measureRight.RangeStatus != 4 && measureRight.RangeMilliMeter < WALL_THRESHOLD_MM) wallRight = true;
    
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
