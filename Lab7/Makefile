# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -g

# Target name
TARGET = Lab7

# Source file
SRC = Lab7.cpp

# Object file
OBJ = Lab7.o

# Default rule
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $(SRC)

# Clean up build files
clean:
	rm -f $(TARGET) $(OBJ)
