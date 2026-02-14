# Compiler and source files
CXX := g++
SRC := $(wildcard src/main.cpp)
OUT := output/typewriter

# Libs and includes
CXXFLAGS = -Wall -g $(shell pkg-config --cflags cairo sdl2 SDL2_ttf SDL2_mixer)
LIBS = $(shell pkg-config --libs cairo sdl2 SDL2_ttf SDL2_mixer)

# Execution
all:
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRC) $(LIBS)