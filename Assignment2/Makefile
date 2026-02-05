CXX = g++
CXXFLAGS = -std=c++17 -Wall -g

all: networkMonitor intfMonitor

networkMonitor: networkMonitor.cpp
	$(CXX) $(CXXFLAGS) -o networkMonitor networkMonitor.cpp

intfMonitor: intfMonitor.cpp
	$(CXX) $(CXXFLAGS) -o intfMonitor intfMonitor.cpp

clean:
	rm -f networkMonitor intfMonitor *.o *.txt $(SOCKET_PATH)
