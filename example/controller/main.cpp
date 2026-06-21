#include <iostream>
#include "mprpccontroller.h"

int main() {
  MprpcController controller;

  std::cout << "failed: " << controller.Failed() << std::endl;

  controller.SetFailed("connect fail");

  if (controller.Failed()) {
    std::cout << "error: " << controller.ErrorText() << std::endl;
  }

  controller.Reset();

  std::cout << "failed after reset: " << controller.Failed() << std::endl;

  return 0;
}