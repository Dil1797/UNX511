# Makefile for etherCtrl.cpp

CXX = g++
CXXFLAGS = -Wall -std=c++11
TARGET = etherCtrl

all: $(TARGET)

$(TARGET): etherCtrl.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) etherCtrl.cpp

clean:
	rm -f $(TARGET)
