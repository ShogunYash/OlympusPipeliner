CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

# Source files
COMMON_SRCS = Processor.cc Register.cc Memory.cc
NOFORWARD_SRCS = MainNoForwarding.cc
FORWARD_SRCS = MainForwarding.cc ForwardingProcessor.cc

# Object files
COMMON_OBJS = $(COMMON_SRCS:.cc=.o)
NOFORWARD_OBJS = $(NOFORWARD_SRCS:.cc=.o)
FORWARD_OBJS = $(FORWARD_SRCS:.cc=.o)

# Header dependencies
DEPS = Processor.hpp Register.hpp Memory.hpp PipelineStages.hpp
FORWARD_DEPS = ForwardingProcessor.hpp $(DEPS)

# Targets
all: noforward forward

noforward: $(COMMON_OBJS) $(NOFORWARD_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

forward: $(COMMON_OBJS) $(FORWARD_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile common source files
%.o: %.cc $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Special rule for forwarding processor files that depend on ForwardingProcessor.hpp
ForwardingProcessor.o: ForwardingProcessor.cc $(FORWARD_DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

MainForwarding.o: MainForwarding.cc $(FORWARD_DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create output directory
outputdir:
	mkdir -p ../outputfiles

clean:
	rm -f *.o noforward forward

# Run targets
run_noforward: noforward
	./noforward $(FILE) $(CYCLES)

run_forward: forward
	./forward $(FILE) $(CYCLES)

# Help message
help:
	@echo "Available targets:"
	@echo "  all           - Build both processors"
	@echo "  noforward     - Build no-forwarding processor only"
	@echo "  forward       - Build forwarding processor only"
	@echo "  run_noforward - Run no-forwarding processor"
	@echo "  run_forward   - Run forwarding processor"
	@echo ""
	@echo "Usage examples:"
	@echo "  make run_noforward FILE=../testfiles/test1.txt CYCLES=20"
	@echo "  make run_forward FILE=../testfiles/test1.txt CYCLES=20"

.PHONY: all clean outputdir help run_noforward run_forward
