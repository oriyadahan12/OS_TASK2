CXX = g++

CXXFLAGS = -Wall -Wextra -std=c++11

TARGETS = mync ttt

all: $(TARGETS)

mync: mync.o
	$(CXX) $(CXXFLAGS) -o mync mync.o

ttt: ttt.o
	$(CXX) $(CXXFLAGS) -o ttt ttt.o

mync.o: mync.cpp
	$(CXX) $(CXXFLAGS) -c mync.cpp

ttt.o: ttt.cpp ttt.hpp
	$(CXX) $(CXXFLAGS) -c ttt.cpp

clean:
	rm -f $(TARGETS) *.o
