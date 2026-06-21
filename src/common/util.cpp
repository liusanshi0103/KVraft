#include "util.h"

std::string helloCommon() {
  return "common module ok";
}
int RandomElectionTimeoutMs() {
  static thread_local std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<int> dist(300, 500);
  return dist(gen);
}

void SleepMs(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}