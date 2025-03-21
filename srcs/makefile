# Makefile for RISC-V Processor Simulator

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g
LDFLAGS =

# Sources for the non-forwarding variant
NOFWD_SRCS = main.cpp processor.cpp no_forwarding.cpp instruction.cpp register_file.cpp memory.cpp instruction_memory.cpp alu.cpp
NOFWD_OBJS = $(NOFWD_SRCS:.cpp=.o)

# Sources for the forwarding variant (if implemented)
FORWARD_SRCS = main_forward.cpp processor.cpp forwarding.cpp instruction.cpp register_file.cpp memory.cpp instruction_memory.cpp alu.cpp
FORWARD_OBJS = $(FORWARD_SRCS:.cpp=.o)

.PHONY: all clean noforward forward

# Default: build both variants
all: noforward forward

# Build the non-forwarding executable (noforward)
noforward: main.o processor.o no_forwarding.o instruction.o register_file.o memory.o instruction_memory.o alu.o
	$(CXX) $(CXXFLAGS) -o noforward main.o processor.o no_forwarding.o instruction.o register_file.o memory.o instruction_memory.o alu.o

# For the forwarding version (using main_forward.cpp)
main_forward.o: main_forward.cpp
	$(CXX) $(CXXFLAGS) -c main_forward.cpp -o main_forward.o

# Build forwarding executable only if forwarding.cpp exists
ifneq ($(wildcard forwarding.cpp),)
forward: main_forward.o processor.o forwarding.o instruction.o register_file.o memory.o instruction_memory.o alu.o
	$(CXX) $(CXXFLAGS) -o forward main_forward.o processor.o forwarding.o instruction.o register_file.o memory.o instruction_memory.o alu.o
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
