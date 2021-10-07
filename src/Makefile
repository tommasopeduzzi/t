CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) -g -O0 -std=c++17
LDFLAGS += $(shell llvm-config --libs --ldflags)

all: main.cpp lexer.cpp parser.cpp error.cpp
	g++ $(CXXFLAGS) -o t main.cpp lexer.cpp parser.cpp error.cpp $(LDFLAGS)

test: all
	./t < test.txt
