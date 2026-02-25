CXX := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -pedantic
INCLUDES := -I.

BIN_DIR := bin
TARGET := bst
SRC := kattis_bst_template.cpp
HDR := SelfBalancingBST.h

.PHONY: all build run clean

all: build

build: $(BIN_DIR)/$(TARGET)

$(BIN_DIR)/$(TARGET): $(SRC) $(HDR)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

run: $(BIN_DIR)/$(TARGET)
	./$(BIN_DIR)/$(TARGET)

clean:
	rm -rf $(BIN_DIR)
