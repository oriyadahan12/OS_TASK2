CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

TARGETS = ttt mync

TTT_SOURCES = ttt.cpp
MYNC_SOURCES = mync.cpp

TTT_OBJECTS = $(TTT_SOURCES:.cpp=.o)
MYNC_OBJECTS = $(MYNC_SOURCES:.cpp=.o)

all: $(TARGETS)

ttt: $(TTT_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

mync: $(MYNC_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(TARGETS) $(TTT_OBJECTS) $(MYNC_OBJECTS)

.PHONY: all clean
