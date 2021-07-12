﻿#include "vector.h"
#include "tests.h"

#include <iostream>
#include <stdexcept>

int main() {
    try {
        Test1();
        Test2();
        Test3();
        Test4();
        Test5();
        Test6();
        Benchmark();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}