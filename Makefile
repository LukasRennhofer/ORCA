BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra

all: $(BIN_DIR)/demo

$(BIN_DIR)/demo: $(OBJ_DIR)/main.o
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJ_DIR)/main.o -o $(BIN_DIR)/demo

$(OBJ_DIR)/main.o: sandbox/main.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c sandbox/main.cpp -o $(OBJ_DIR)/main.o

clean:
	rm -rf $(BUILD_DIR)

run: $(BIN_DIR)/demo
	./$(BIN_DIR)/demo

.PHONY: all clean run