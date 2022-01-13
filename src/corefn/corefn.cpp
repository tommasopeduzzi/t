//
// Created by Tommaso Peduzzi on 20.11.21.
//

#include "corefn.h"
#include <iostream>

using namespace std;

extern "C"  void printString(const char* str){
    cout << str;
}

extern "C" void printAscii(double c){
    cout << (char)c;
}

extern "C" void printNumber(double number){
    cout << number;
}

extern "C" char *input(){
    string str;
    getline(cin, str);
    return strdup(str.c_str());
}