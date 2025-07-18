CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./src
SRC = src/main.c src/lexer.c src/parser.c src/semantic.c src/codegen.c src/vm.c
OUT = build/katlang

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)