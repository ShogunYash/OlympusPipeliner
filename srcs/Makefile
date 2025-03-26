CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

SRCS = main_noforward.cpp Processor.cpp Register.cpp Memory.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = Processor.hpp Register.hpp Memory.hpp PipelineStages.hpp

# Targets
all: noforward

noforward: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o noforward
