CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
TARGET = main
SOURCES = main.cpp Lexer.cpp

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: clean run