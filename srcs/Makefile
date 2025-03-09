# Makefile for RISC-V processor simulator

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Targets
all: noforward forward

noforward: noforward.o processor.o no_forwarding_processor.o
	$(CXX) $(CXXFLAGS) -o $@ $^

forward: forward.o processor.o forwarding_processor.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Object files
noforward.o: noforward.cpp no_forwarding_processor.hpp processor.hpp
	$(CXX) $(CXXFLAGS) -c $<

forward.o: forward.cpp forwarding_processor.hpp processor.hpp
	$(CXX) $(CXXFLAGS) -c $<

processor.o: processor.cpp processor.hpp
	$(CXX) $(CXXFLAGS) -c $<

no_forwarding_processor.o: no_forwarding_processor.cpp no_forwarding_processor.hpp processor.hpp
	$(CXX) $(CXXFLAGS) -c $<

forwarding_processor.o: forwarding_processor.cpp forwarding_processor.hpp processor.hpp
	$(CXX) $(CXXFLAGS) -c $<

# Clean
clean:
	rm -f *.o noforward forward

.PHONY: all clean