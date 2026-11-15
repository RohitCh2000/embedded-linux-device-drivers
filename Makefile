# Makefile - simulated-register HAL demo
CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
SRC_DIR := src
BUILD   := build
TARGET  := $(BUILD)/hal_demo
SRCS    := $(SRC_DIR)/main.c $(SRC_DIR)/hal.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS) $(SRC_DIR)/hal.h
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -o $@ $(SRCS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD)
