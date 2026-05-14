#include "API.h"
#include <iostream>

int API::mazeWidth() { std::cout << "mazeWidth" << std::endl; std::string r; std::cin >> r; return std::stoi(r); }
int API::mazeHeight() { std::cout << "mazeHeight" << std::endl; std::string r; std::cin >> r; return std::stoi(r); }
bool API::wallFront() { std::cout << "wallFront" << std::endl; std::string r; std::cin >> r; return r == "true"; }
bool API::wallRight() { std::cout << "wallRight" << std::endl; std::string r; std::cin >> r; return r == "true"; }
bool API::wallLeft() { std::cout << "wallLeft" << std::endl; std::string r; std::cin >> r; return r == "true"; }
void API::moveForward(int d) { std::cout << "moveForward " << d << std::endl; std::string r; std::cin >> r; }
void API::turnRight() { std::cout << "turnRight" << std::endl; std::string r; std::cin >> r; }
void API::turnLeft() { std::cout << "turnLeft" << std::endl; std::string r; std::cin >> r; }
void API::setWall(int x, int y, char d) { std::cout << "setWall " << x << " " << y << " " << d << std::endl; }
void API::clearWall(int x, int y, char d) { std::cout << "clearWall " << x << " " << y << " " << d << std::endl; }
void API::setColor(int x, int y, char c) { std::cout << "setColor " << x << " " << y << " " << c << std::endl; }
void API::clearColor(int x, int y) { std::cout << "clearColor " << x << " " << y << std::endl; }
void API::clearAllColor() { std::cout << "clearAllColor" << std::endl; }
void API::setText(int x, int y, const std::string& t) { std::cout << "setText " << x << " " << y << " " << t << std::endl; }
void API::clearText(int x, int y) { std::cout << "clearText " << x << " " << y << std::endl; }
void API::clearAllText() { std::cout << "clearAllText" << std::endl; }
bool API::wasReset() { std::cout << "wasReset" << std::endl; std::string r; std::cin >> r; return r == "true"; }
void API::ackReset() { std::cout << "ackReset" << std::endl; std::string r; std::cin >> r; }
