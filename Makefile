CXX := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -pedantic
INCLUDES := -I.

BIN_DIR := bin
TARGET := ast_program
SRC := main.cpp AST.cpp
HDR := AST.h

.PHONY: all build run clean

all: build

build: $(BIN_DIR)/$(TARGET)

$(BIN_DIR)/$(TARGET): $(SRC) $(HDR)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $@

run: $(BIN_DIR)/$(TARGET)
	./$(BIN_DIR)/$(TARGET)

clean:
	rm -rf $(BIN_DIR)
