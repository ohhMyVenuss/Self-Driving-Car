#include "robot_common.h"
#include "maze_solver.h"

// Định nghĩa chân cảm biến va chạm C1 và C2
// (Giả sử bạn dùng công tắc hành trình hoặc cảm biến tiệm cận)
#define SENSOR_C1_PIN 5  
#define SENSOR_C2_PIN 15 

enum RobotMode {
  MODE_LINE_FOLLOWER,
  MODE_MAZE_SOLVER,
  MODE_STOP
};

RobotMode currentMode = MODE_LINE_FOLLOWER;

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo các chân tín hiệu C1, C2
  pinMode(SENSOR_C1_PIN, INPUT_PULLUP);
  pinMode(SENSOR_C2_PIN, INPUT_PULLUP);

  setupLineFollower();
  setupMazeSolver();
  
  Serial.println("System Ready. Mode: LINE FOLLOWER");
}

void loop() {
  // 1. Kiểm tra húc vào trạm C1 -> Chuyển sang Dò Mê Cung
  if (currentMode == MODE_LINE_FOLLOWER) {
    if (digitalRead(SENSOR_C1_PIN) == LOW) {
      delay(50); // Chống dội nút
      if (digitalRead(SENSOR_C1_PIN) == LOW) {
        currentMode = MODE_MAZE_SOLVER;
        setMotor(0, 0); // Dừng lại một nhịp trước khi đổi thuật toán
        Serial.println("!!! HIT C1: SWITCHED TO MAZE SOLVER !!!");
        delay(1000); 
      }
    }
  }
  
  // 2. Kiểm tra húc vào trạm C2 (sau khi đã giải xong mê cung) -> Dừng hoàn toàn
  if (currentMode == MODE_MAZE_SOLVER) {
    if (digitalRead(SENSOR_C2_PIN) == LOW) {
      delay(50);
      if (digitalRead(SENSOR_C2_PIN) == LOW) {
        currentMode = MODE_STOP;
        setMotor(0, 0); // Đã chạm đích C2, dừng động cơ
        Serial.println("!!! HIT C2: MAZE FINISHED. STOPPING !!!");
      }
    }
  }

  // 3. Chạy thuật toán tùy theo Mode
  if (currentMode == MODE_LINE_FOLLOWER) {
    loopLineFollower();
  } else if (currentMode == MODE_MAZE_SOLVER) {
    loopMazeSolver();
  } else if (currentMode == MODE_STOP) {
    setMotor(0, 0); // Đảm bảo xe luôn dừng
  }
}
