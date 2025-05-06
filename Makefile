# Makefile for Block Breaker Game
# Uses GTK3 and Cairo

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2
GTK_FLAGS = `pkg-config --cflags --libs gtk+-3.0`

# Target executable name
TARGET = block_breaker

# Source files
SRCS = block_breaker.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Link the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTK_FLAGS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(GTK_FLAGS)

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)

# Run the game
run: $(TARGET)
	./$(TARGET)

# Debug build with debug symbols and no optimization
debug: CXXFLAGS = -Wall -Wextra -std=c++17 -g -O0
debug: clean all

# Install the game to /usr/local/bin (requires sudo)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall the game
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Help target
help:
	@echo "Makefile for Block Breaker Game"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the game (default target)"
	@echo "  clean     - Remove build files"
	@echo "  run       - Build and run the game"
	@echo "  debug     - Build with debug symbols"
	@echo "  install   - Install the game to /usr/local/bin"
	@echo "  uninstall - Remove the game from /usr/local/bin"
	@echo "  help      - Display this help message"

# Phony targets
.PHONY: all clean run debug install uninstall help
