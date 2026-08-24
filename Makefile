# Cross-platform Makefile for raylib
# macOS uses your exact framework setup
# Linux uses X11
# Windows assumes MinGW/MSYS2

CXX := g++
TARGET := TheGame
SRC := $(shell find . -name "*.cpp" -not -path "./vendor/*")

UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(OS),Windows_NT)
    PLATFORM := WINDOWS
else ifeq ($(UNAME_S),Darwin)
    PLATFORM := MACOS
else
    PLATFORM := LINUX
endif

CXXFLAGS := -std=c++17

# Internal (vendored) raylib — see vendor/ and setup-deps.sh.
# If vendor/raylib exists it is preferred; otherwise fall back to a system install.
VENDOR_RAYLIB := vendor/raylib

# Internal (vendored) Steamworks SDK — see vendor/ and setup-steamworks.sh.
# Picked up automatically when present; builds without it are unaffected.
# Guard Steam code with #ifdef STEAMWORKS_AVAILABLE.
VENDOR_STEAM := vendor/steamworks
HAS_STEAM := $(wildcard $(VENDOR_STEAM)/include/steam/steam_api.h)

ifeq ($(PLATFORM),MACOS)
    TARGET := TheGame
    ifneq ($(wildcard $(VENDOR_RAYLIB)/include/raylib.h),)
        INCLUDES := -I$(VENDOR_RAYLIB)/include
        LIBDIRS  := -L$(VENDOR_RAYLIB)/lib
    else
        INCLUDES := -I/opt/homebrew/include
        LIBDIRS  := -L/opt/homebrew/lib
    endif
    LIBS     := -lraylib \
                -framework OpenGL \
                -framework Cocoa \
                -framework IOKit \
                -framework CoreVideo
endif

ifeq ($(PLATFORM),LINUX)
    TARGET := TheGame
    ifneq ($(wildcard $(VENDOR_RAYLIB)/include/raylib.h),)
        INCLUDES := -I$(VENDOR_RAYLIB)/include
        LIBDIRS  := -L$(VENDOR_RAYLIB)/lib
    else
        INCLUDES := -I/usr/local/include
        LIBDIRS  := -L/usr/local/lib
    endif
    LIBS     := -lraylib -lm -lpthread -ldl -lrt -lX11
endif

ifeq ($(PLATFORM),WINDOWS)
    TARGET := TheGame.exe
    ifneq ($(wildcard $(VENDOR_RAYLIB)/include/raylib.h),)
        INCLUDES := -I$(VENDOR_RAYLIB)/include
        LIBDIRS  := -L$(VENDOR_RAYLIB)/lib
    else
        INCLUDES :=
        LIBDIRS  :=
    endif
    LIBS     := -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32
endif

ifneq ($(HAS_STEAM),)
    INCLUDES += -I$(VENDOR_STEAM)/include
    CXXFLAGS += -DSTEAMWORKS_AVAILABLE=1
    ifeq ($(PLATFORM),WINDOWS)
        # The SDK ships an MSVC-format import lib, so link the DLL directly.
        LIBS += $(VENDOR_STEAM)/bin/steam_api64.dll
        STEAM_RUNTIME := $(VENDOR_STEAM)/bin/steam_api64.dll
    endif
    ifeq ($(PLATFORM),LINUX)
        LIBS += -L$(VENDOR_STEAM)/lib -lsteam_api -Wl,-rpath,'$$ORIGIN'
        STEAM_RUNTIME := $(VENDOR_STEAM)/lib/libsteam_api.so
    endif
    ifeq ($(PLATFORM),MACOS)
        LIBS += -L$(VENDOR_STEAM)/lib -lsteam_api -Wl,-rpath,@loader_path
        STEAM_RUNTIME := $(VENDOR_STEAM)/lib/libsteam_api.dylib
    endif
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(SRC) -o $(TARGET) $(CXXFLAGS) $(INCLUDES) $(LIBDIRS) $(LIBS)
ifneq ($(HAS_STEAM),)
	cp -f $(STEAM_RUNTIME) .
endif

run: $(TARGET)
	./$(TARGET)

build-run: $(TARGET)
	./$(TARGET)

rebuild-run: clean $(TARGET)
	./$(TARGET)

clean:
	rm -f game game.exe
