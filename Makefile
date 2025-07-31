# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -O3 -fopenmp -Wall -Isrc -Iutils

# Directories
SRC_DIR := src
UTIL_DIR := utils
OBJ_DIR := build/obj
BIN_DIR := build
BIN := $(BIN_DIR)/niorca

# Installation directories
PREFIX ?= /usr/local
INSTALL_BIN_DIR := $(DESTDIR)$(PREFIX)/bin

# Source files
SRCS := $(SRC_DIR)/ORCA.cpp \
        $(SRC_DIR)/NIORCA.cpp \
        $(UTIL_DIR)/SEQ-Counter.cpp \
        $(UTIL_DIR)/OMP-Vertex-Counter.cpp \
        $(UTIL_DIR)/OMP-Thread-Counter.cpp \
        driver.cpp

# Object files (flattened, e.g., build/obj/ORCA.o)
OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(SRCS)))

# Default target
all: $(BIN)

# Link object files into final binary
$(BIN): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files from src/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile source files from utils/
$(OBJ_DIR)/%.o: $(UTIL_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Install target
install: all
	@mkdir -p $(INSTALL_BIN_DIR)
	cp $(BIN) $(INSTALL_BIN_DIR)/niorca

# Uninstall target
uninstall:
	rm -f $(PREFIX)/bin/niorca

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

.PHONY: all clean install uninstall

