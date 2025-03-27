CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

# Source files
COMMON_SRCS = Processor.cc Register.cc Memory.cc
NOFORWARD_SRCS = MainNoForwarding.cc
FORWARD_SRCS = MainForwarding.cc ForwardingProcessor.cc
# DISASM_SRCS = RiscVDisassembler.cc

# Object files
COMMON_OBJS = $(COMMON_SRCS:.cc=.o)
NOFORWARD_OBJS = $(NOFORWARD_SRCS:.cc=.o)
FORWARD_OBJS = $(FORWARD_SRCS:.cc=.o)
DISASM_OBJS = $(DISASM_SRCS:.cc=.o)

# Header dependencies
DEPS = Processor.hpp Register.hpp Memory.hpp PipelineStages.hpp
FORWARD_DEPS = ForwardingProcessor.hpp $(DEPS)

# Targets
all: noforward forward 

noforward: $(COMMON_OBJS) $(NOFORWARD_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

forward: $(COMMON_OBJS) $(FORWARD_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# disasm: $(DISASM_OBJS)
# 	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile common source files
%.o: %.cc $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Special compilation rules
ForwardingProcessor.o: ForwardingProcessor.cc $(FORWARD_DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

MainForwarding.o: MainForwarding.cc $(FORWARD_DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# RiscVDisassembler.o: RiscVDisassembler.cc
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create output directory
outputdir:
	mkdir -p ../outputfiles

clean:
	rm -f *.o noforward forward disasm

# Run targets
run_noforward: noforward
	./noforward $(FILE) $(CYCLES)

run_forward: forward
	./forward $(FILE) $(CYCLES)

# run_disasm: disasm
# 	./disasm $(INPUT) $(OUTPUT)

# Help message
help:
	@echo "Available targets:"
	@echo "  all           - Build all executables"
	@echo "  noforward     - Build no-forwarding processor"
	@echo "  forward       - Build forwarding processor"
	@echo "  disasm        - Build RISC-V disassembler"
	@echo "  run_noforward - Run no-forwarding processor"
	@echo "  run_forward   - Run forwarding processor"
	@echo "  run_disasm    - Run RISC-V disassembler"
	@echo ""
	@echo "Usage examples:"
	@echo "  make run_noforward FILE=../testfiles/test1.txt CYCLES=20"
	@echo "  make run_forward FILE=../testfiles/test1.txt CYCLES=20" 
	@echo "  make run_disasm INPUT=hexcode.txt OUTPUT=disassembled.txt"
	@echo "  make run_disasm INPUT=hexcode.txt  # Output to screen"

.PHONY: all clean outputdir help run_noforward run_forward run_disasm
