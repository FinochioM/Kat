CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = build/kat
SRC_DIR = src
BUILD_DIR = build
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))

.PHONY: all clean test install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)	
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /Q build\*.o build\kat.exe 2>nul || echo "Files cleaned"

test: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	mkdir -p ../bin
	cp $(TARGET) ../bin/
