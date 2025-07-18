CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = build/kat
SOURCES = src/main.c src/lexer.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	mkdir -p ../bin
	cp $(TARGET) ../bin/

.SUFFIXES: .c .o