//
// Created by Tommaso Peduzzi on 20.11.21.
//

#include "corefn.h"
#include <iostream>

extern "C"  void printString(const char* str){
    std::cout << str;
}

extern "C" void printAscii(double c){
    std::cout << (char)c;
}

extern "C" void printNumber(double number){
    std::cout << number;
}

extern "C" char *input(){
    std::string str;
    std::getline(std::cin, str);
    return strdup(str.c_str());
}