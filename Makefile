CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++17 -Isrc
SRC_DIR = src
BIN_DIR = bin
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

ifdef STATIC
    CXXFLAGS += -DSFML_STATIC
    SFML_LIBS = -L/usr/local/lib -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -lfreetype -lGL -lX11 -lXrandr -lXcursor -lXi -ludev -lpthread
else
    SFML_LIBS = -L/usr/local/lib -lsfml-graphics -lsfml-window -lsfml-system
endif

OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%.o,$(SRCS))

ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    TARGET = $(BIN_DIR)\main.exe
    MKDIR_CMD = if not exist $(BIN_DIR) mkdir $(BIN_DIR)
    CLEAN_CMD = if exist $(BIN_DIR) del /Q /S $(BIN_DIR)\*.*
    RUN_CMD = $(BIN_DIR)\main.exe
else
    TARGET = $(BIN_DIR)/main
    MKDIR_CMD = mkdir -p $(BIN_DIR)
    CLEAN_CMD = rm -rf $(BIN_DIR)/*
    RUN_CMD = ./$(BIN_DIR)/main
endif

all: prebuild $(TARGET)

prebuild:
	@$(MKDIR_CMD)

$(TARGET): $(OBJS)
	@echo Linking $@
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(SFML_LIBS)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo Compiling $<
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo Cleaning up...
	@$(CLEAN_CMD)
	@echo Clean completed.

run: all
	@echo Running $(TARGET)...
	@$(RUN_CMD)

.PHONY: all prebuild clean run
