#include <iostream>
#include <string>
#include "API.h"

#define MAX_MAZE_SIZE 32
#define WALL_NORTH 1
#define WALL_EAST  2
#define WALL_SOUTH 4
#define WALL_WEST  8

enum Heading { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 };

int main() {
    API::setColor(0, 0, 'G');
    API::setText(0, 0, "START");

    // Lấy kích thước mê cung tự động từ phần mềm MMS (mặc định 16x16)
    int mazeSize = API::mazeWidth();
    
    Heading currentHeading = NORTH;
    int posX = 0;
    int posY = 0;
    
    // Đặt đích đến là góc trên cùng bên phải
    int targetX = mazeSize - 1;
    int targetY = mazeSize - 1;

    int distances[MAX_MAZE_SIZE][MAX_MAZE_SIZE];
    int walls[MAX_MAZE_SIZE][MAX_MAZE_SIZE];

    // Khởi tạo ranh giới ngoài cùng của mê cung
    for (int y = 0; y < mazeSize; y++) {
        for (int x = 0; x < mazeSize; x++) {
            walls[y][x] = 0;
            if (y == 0) walls[y][x] |= WALL_SOUTH;
            if (y == mazeSize - 1) walls[y][x] |= WALL_NORTH;
            if (x == 0) walls[y][x] |= WALL_WEST;
            if (x == mazeSize - 1) walls[y][x] |= WALL_EAST;
        }
    }

    while (true) {
        // 1. ĐỌC CẢM BIẾN TỪ MMS VÀ VẼ TƯỜNG LÊN MÀN HÌNH
        bool wallFront = API::wallFront();
        bool wallRight = API::wallRight();
        bool wallLeft = API::wallLeft();
        
        int currentWallData = 0;
        
        if (currentHeading == NORTH) {
            if (wallFront) { currentWallData |= WALL_NORTH; API::setWall(posX, posY, 'n'); if (posY + 1 < mazeSize) walls[posY + 1][posX] |= WALL_SOUTH; }
            if (wallRight) { currentWallData |= WALL_EAST; API::setWall(posX, posY, 'e'); if (posX + 1 < mazeSize) walls[posY][posX + 1] |= WALL_WEST; }
            if (wallLeft)  { currentWallData |= WALL_WEST; API::setWall(posX, posY, 'w'); if (posX - 1 >= 0) walls[posY][posX - 1] |= WALL_EAST; }
        } else if (currentHeading == EAST) {
            if (wallFront) { currentWallData |= WALL_EAST; API::setWall(posX, posY, 'e'); if (posX + 1 < mazeSize) walls[posY][posX + 1] |= WALL_WEST; }
            if (wallRight) { currentWallData |= WALL_SOUTH; API::setWall(posX, posY, 's'); if (posY - 1 >= 0) walls[posY - 1][posX] |= WALL_NORTH; }
            if (wallLeft)  { currentWallData |= WALL_NORTH; API::setWall(posX, posY, 'n'); if (posY + 1 < mazeSize) walls[posY + 1][posX] |= WALL_SOUTH; }
        } else if (currentHeading == SOUTH) {
            if (wallFront) { currentWallData |= WALL_SOUTH; API::setWall(posX, posY, 's'); if (posY - 1 >= 0) walls[posY - 1][posX] |= WALL_NORTH; }
            if (wallRight) { currentWallData |= WALL_WEST; API::setWall(posX, posY, 'w'); if (posX - 1 >= 0) walls[posY][posX - 1] |= WALL_EAST; }
            if (wallLeft)  { currentWallData |= WALL_EAST; API::setWall(posX, posY, 'e'); if (posX + 1 < mazeSize) walls[posY][posX + 1] |= WALL_WEST; }
        } else if (currentHeading == WEST) {
            if (wallFront) { currentWallData |= WALL_WEST; API::setWall(posX, posY, 'w'); if (posX - 1 >= 0) walls[posY][posX - 1] |= WALL_EAST; }
            if (wallRight) { currentWallData |= WALL_NORTH; API::setWall(posX, posY, 'n'); if (posY + 1 < mazeSize) walls[posY + 1][posX] |= WALL_SOUTH; }
            if (wallLeft)  { currentWallData |= WALL_SOUTH; API::setWall(posX, posY, 's'); if (posY - 1 >= 0) walls[posY - 1][posX] |= WALL_NORTH; }
        }
        
        walls[posY][posX] |= currentWallData; 

        // 2. TÍNH LẠI FLOOD FILL
        for (int y = 0; y < mazeSize; y++) {
            for (int x = 0; x < mazeSize; x++) {
                distances[y][x] = 255;
            }
        }
        distances[targetY][targetX] = 0;
        
        struct Point { int x, y; };
        Point queue[MAX_MAZE_SIZE * MAX_MAZE_SIZE];
        int head = 0, tail = 0;
        queue[tail++] = {targetX, targetY};
        
        while (head < tail) {
            Point p = queue[head++];
            int d = distances[p.y][p.x];
            
            if (p.y + 1 < mazeSize && !(walls[p.y][p.x] & WALL_NORTH)) {
                if (distances[p.y + 1][p.x] == 255) { distances[p.y + 1][p.x] = d + 1; queue[tail++] = {p.x, p.y + 1}; }
            }
            if (p.y - 1 >= 0 && !(walls[p.y][p.x] & WALL_SOUTH)) {
                if (distances[p.y - 1][p.x] == 255) { distances[p.y - 1][p.x] = d + 1; queue[tail++] = {p.x, p.y - 1}; }
            }
            if (p.x + 1 < mazeSize && !(walls[p.y][p.x] & WALL_EAST)) {
                if (distances[p.y][p.x + 1] == 255) { distances[p.y][p.x + 1] = d + 1; queue[tail++] = {p.x + 1, p.y}; }
            }
            if (p.x - 1 >= 0 && !(walls[p.y][p.x] & WALL_WEST)) {
                if (distances[p.y][p.x - 1] == 255) { distances[p.y][p.x - 1] = d + 1; queue[tail++] = {p.x - 1, p.y}; }
            }
        }

        // Hiện số khoảng cách lên MMS cho dễ nhìn
        for(int i=0; i<mazeSize; i++){
            for(int j=0; j<mazeSize; j++){
                API::setText(j, i, std::to_string(distances[i][j]));
            }
        }

        // 3. NẾU ĐÃ TỚI ĐÍCH THÌ DỪNG LẠI
        if (posX == targetX && posY == targetY) {
            std::cerr << "Hoan Thanh Me Cung!" << std::endl;
            break; 
        }

        // 4. CHỌN HƯỚNG CÓ SỐ NHỎ NHẤT ĐỂ ĐI
        int distFront = 255, distRight = 255, distLeft = 255, distBack = 255;
        
        if (currentHeading == NORTH) {
            if (!(walls[posY][posX] & WALL_NORTH) && posY + 1 < mazeSize) distFront = distances[posY + 1][posX];
            if (!(walls[posY][posX] & WALL_EAST) && posX + 1 < mazeSize) distRight = distances[posY][posX + 1];
            if (!(walls[posY][posX] & WALL_WEST) && posX - 1 >= 0) distLeft = distances[posY][posX - 1];
            if (!(walls[posY][posX] & WALL_SOUTH) && posY - 1 >= 0) distBack = distances[posY - 1][posX];
        } else if (currentHeading == EAST) {
            if (!(walls[posY][posX] & WALL_EAST) && posX + 1 < mazeSize) distFront = distances[posY][posX + 1];
            if (!(walls[posY][posX] & WALL_SOUTH) && posY - 1 >= 0) distRight = distances[posY - 1][posX];
            if (!(walls[posY][posX] & WALL_NORTH) && posY + 1 < mazeSize) distLeft = distances[posY + 1][posX];
            if (!(walls[posY][posX] & WALL_WEST) && posX - 1 >= 0) distBack = distances[posY][posX - 1];
        } else if (currentHeading == SOUTH) {
            if (!(walls[posY][posX] & WALL_SOUTH) && posY - 1 >= 0) distFront = distances[posY - 1][posX];
            if (!(walls[posY][posX] & WALL_WEST) && posX - 1 >= 0) distRight = distances[posY][posX - 1];
            if (!(walls[posY][posX] & WALL_EAST) && posX + 1 < mazeSize) distLeft = distances[posY][posX + 1];
            if (!(walls[posY][posX] & WALL_NORTH) && posY + 1 < mazeSize) distBack = distances[posY + 1][posX];
        } else if (currentHeading == WEST) {
            if (!(walls[posY][posX] & WALL_WEST) && posX - 1 >= 0) distFront = distances[posY][posX - 1];
            if (!(walls[posY][posX] & WALL_NORTH) && posY + 1 < mazeSize) distRight = distances[posY + 1][posX];
            if (!(walls[posY][posX] & WALL_SOUTH) && posY - 1 >= 0) distLeft = distances[posY - 1][posX];
            if (!(walls[posY][posX] & WALL_EAST) && posX + 1 < mazeSize) distBack = distances[posY][posX + 1];
        }
        
        int minDist = 255;
        if (distFront < minDist) minDist = distFront;
        if (distRight < minDist) minDist = distRight;
        if (distLeft < minDist) minDist = distLeft;
        if (distBack < minDist) minDist = distBack;
        
        // DI CHUYỂN
        if (minDist == distFront && distFront != 255) {
            API::moveForward();
            if (currentHeading == NORTH) posY++; else if (currentHeading == SOUTH) posY--; else if (currentHeading == EAST) posX++; else if (currentHeading == WEST) posX--;
        } else if (minDist == distRight && distRight != 255) {
            API::turnRight();
            currentHeading = (Heading)((currentHeading + 1) % 4);
            API::moveForward();
            if (currentHeading == NORTH) posY++; else if (currentHeading == SOUTH) posY--; else if (currentHeading == EAST) posX++; else if (currentHeading == WEST) posX--;
        } else if (minDist == distLeft && distLeft != 255) {
            API::turnLeft();
            currentHeading = (Heading)((currentHeading + 3) % 4);
            API::moveForward();
            if (currentHeading == NORTH) posY++; else if (currentHeading == SOUTH) posY--; else if (currentHeading == EAST) posX++; else if (currentHeading == WEST) posX--;
        } else {
            API::turnRight();
            API::turnRight();
            currentHeading = (Heading)((currentHeading + 2) % 4);
            API::moveForward();
            if (currentHeading == NORTH) posY++; else if (currentHeading == SOUTH) posY--; else if (currentHeading == EAST) posX++; else if (currentHeading == WEST) posX--;
        }
    }
}
