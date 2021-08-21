all: test

test: main.cpp lexer.cpp parser.cpp error.cpp
	g++ -std=c++17 -o t main.cpp lexer.cpp parser.cpp error.cpp
	./t < test.txt

