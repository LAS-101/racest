CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Isrc
LDFLAGS := -lraylib -pthread -lm -ldl

SRCS := src/main.cpp src/game.cpp src/renderer.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET := racest

.PHONY: all build run clean web serve-web

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(TARGET)

# Activate emsdk_env.sh first. CMake builds a separate WebAssembly raylib.
web:
	emcmake cmake -S . -B build/web -DCMAKE_BUILD_TYPE=Release
	cmake --build build/web --parallel

serve-web: web
	python3 -m http.server 8000 --bind 127.0.0.1 --directory build/web/public

clean:
	rm -f $(OBJS) main.o game.o renderer.o $(TARGET)
