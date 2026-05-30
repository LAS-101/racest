CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Isrc
LDFLAGS := -lraylib -pthread -lm -ldl

SRCS := src/main.cpp src/game.cpp src/renderer.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET := racest

.PHONY: all build run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -f $(OBJS) main.o game.o renderer.o $(TARGET)
