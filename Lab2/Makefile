CXX = g++
CXXFLAGS = -Wall -std=c++11 -I.
LDFLAGS = -std=c++11

TARGET = Lab2
SRC = Lab2_Dil.cpp
LIB = libPidUtil.a

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(LIB) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)
