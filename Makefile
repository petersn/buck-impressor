
CXXFLAGS=-Ofast -std=c++17 -Wall -Wextra -g -lasound -pthread 

all: synth

synth: synth.o
	$(CXX) $(CXXFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f synth *.o

