CPPFLAGS := -std=c++20 -Wall -Wextra -Iinclude -I/path/to/argparse/include
LIBS := -lpaho-mqttpp3 -lpaho-mqtt3as -lpqxx -lpq -lcjson
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET := $(BIN_DIR)/mqtt_listener

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p $(@D)
	$(CXX) $(OBJS) -o $@ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(INC_DIR)/*.hpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
