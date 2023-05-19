#include <iostream>

#include "gpio/gpio.hpp"

int main() {
    const int kNumber = 115;
    gsync::Gpio gpio(kNumber);

    std::cout << "GPIO Number: " << gpio.SysFsNumber() << std::endl;
    std::cout << "Default Direction: "
              << ((gsync::Gpio::Direction::kInput == gpio.Dir()) ? "in" : "out")
              << std::endl;
    std::cout << "Default Value: "
              << ((gsync::Gpio::Value::kLow == gpio.Val()) ? "0" : "1")
              << std::endl;

    gpio.EdgeType(gsync::Gpio::Edge::kRising);
    std::cout << "Set Edge Type to Rising" << std::endl;

    std::cout << "Waiting for Trigger Event..." << std::endl;
    gpio.WaitForEdge();
    std::cout << "Detected Event!" << std::endl;

    return 0;
}
