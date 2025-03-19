# Makefile for RISC-V Processor Simulator

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g
LDFLAGS =

# Sources for the non-forwarding variant
NOFWD_SRCS = main.cpp processor.cpp no_forwarding.cpp instruction.cpp register_file.cpp memory.cpp alu.cpp
NOFWD_OBJS = $(NOFWD_SRCS:.cpp=.o)

# Sources for the forwarding variant (if implemented)
FORWARD_SRCS = main.cpp processor.cpp forwarding.cpp instruction.cpp register_file.cpp memory.cpp alu.cpp
FORWARD_OBJS = $(FORWARD_SRCS:.cpp=.o)

.PHONY: all clean noforward forward

# Default: build both variants
all: noforward forward

# Build the non-forwarding executable (noforward)
noforward: $(NOFWD_OBJS)
	$(CXX) $(CXXFLAGS) -o noforward $(NOFWD_OBJS) $(LDFLAGS)

# Build forwarding executable only if forwarding.cpp exists
ifneq ($(wildcard forwarding.cpp),)
forward: $(FORWARD_OBJS)
	$(CXX) $(CXXFLAGS) -o forward $(FORWARD_OBJS) $(LDFLAGS)
else
forward:
	@echo "Forward variant not implemented: forwarding.cpp not found."
endif

# Generic rule for compiling .cpp files to .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean built object files and executables
clean:
	rm -f *.o noforward forward
