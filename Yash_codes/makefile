CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

# Common source files
COMMON_SRCS = Processor.cpp Register.cpp Memory.cpp

# NoForwarding-specific source files
NOFORWARD_SRCS = main.cpp $(COMMON_SRCS)
NOFORWARD_OBJS = $(NOFORWARD_SRCS:.cpp=.o)

# Forwarding-specific source files
FORWARD_SRCS = mainForward.cpp ForwardingProcessor.cpp $(COMMON_SRCS)
FORWARD_OBJS = $(FORWARD_SRCS:.cpp=.o)

# Header dependencies
DEPS = Processor.hpp Register.hpp Memory.hpp PipelineStages.hpp ForwardingProcessor.hpp

# Targets
all: noforward forward

noforward: $(NOFORWARD_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

forward: $(FORWARD_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o noforward forward