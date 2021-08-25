CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) -g -O0 -std=c++17
LDFLAGS += $(shell llvm-config --libs --ldflags)

all: test

test: main.cpp lexer.cpp parser.cpp error.cpp
	g++ $(CXXFLAGS) -o t main.cpp lexer.cpp parser.cpp error.cpp $(LDFLAGS)
	./t < test.txt