# Build:  make        Run:  ./library        Self-test:  make test
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude
SRC      = $(wildcard src/*.cpp)

library: $(SRC) $(wildcard include/*.h)
	$(CXX) $(CXXFLAGS) $(SRC) -o library

test: library
	./library --demo

clean:
	rm -f library library.exe
