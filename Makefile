# Cross-platform Makefile for raylib
# macOS uses your exact framework setup
# Linux uses X11
# Windows assumes MinGW/MSYS2

CXX := g++
TARGET := TheGame
SRC := $(shell find . -name "*.cpp")

UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(OS),Windows_NT)
    PLATFORM := WINDOWS
else ifeq ($(UNAME_S),Darwin)
    PLATFORM := MACOS
else
    PLATFORM := LINUX
endif

CXXFLAGS := -std=c++17

ifeq ($(PLATFORM),MACOS)
    TARGET := TheGame
    INCLUDES := -I/opt/homebrew/include
    LIBDIRS  := -L/opt/homebrew/lib
    LIBS     := -lraylib \
                -framework OpenGL \
                -framework Cocoa \
                -framework IOKit \
                -framework CoreVideo
endif

ifeq ($(PLATFORM),LINUX)
    TARGET := TheGame
    INCLUDES := -I/usr/local/include
    LIBDIRS  := -L/usr/local/lib
    LIBS     := -lraylib -lm -lpthread -ldl -lrt -lX11
endif

ifeq ($(PLATFORM),WINDOWS)
    TARGET := TheGame.exe
    INCLUDES :=
    LIBDIRS  :=
    LIBS     := -lraylib -lopengl32 -lgdi32 -lwinmm
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(SRC) -o $(TARGET) $(CXXFLAGS) $(INCLUDES) $(LIBDIRS) $(LIBS)

run: $(TARGET)
	./$(TARGET)

build-run: $(TARGET)
	./$(TARGET)

rebuild-run: clean $(TARGET)
	./$(TARGET)

clean:
	rm -f TheGame TheGame.exe
