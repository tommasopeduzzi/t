//
// Created by Tommaso Peduzzi on 20.11.21.
//

#include "corefn.h"
#include <iostream>

extern "C"  double printString(const char* str){
    std::cout << str;
    return 0;
}

extern "C" double printAscii(double c){
    std::cout << (char)c;
    return 0;
}

extern "C" double printNumber(double number){
    std::cout << number;
    return 0;
}

extern "C" char *input(){
    std::string str;
    std::getline(std::cin, str);
    return strdup(str.c_str());
}