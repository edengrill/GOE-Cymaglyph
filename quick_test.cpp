// Quick compilation test
#include <iostream>
#include <cmath>

// Test that our header files are valid C++
#include "Source/ModeTables.h"

int main() {
    std::cout << "Testing ModeTables..." << std::endl;
    
    auto squareModes = CymaglyphModes::getSquareModes();
    std::cout << "Square modes count: " << squareModes.size() << std::endl;
    
    auto circularModes = CymaglyphModes::getCircularModes();
    std::cout << "Circular modes count: " << circularModes.size() << std::endl;
    
    float rank = CymaglyphModes::frequencyToModeRank(440.0f);
    std::cout << "440Hz mode rank: " << rank << std::endl;
    
    auto waterMode = CymaglyphModes::getWaterMode(440.0f);
    std::cout << "Water mode n: " << waterMode.n << std::endl;
    
    std::cout << "All tests passed!" << std::endl;
    return 0;
}