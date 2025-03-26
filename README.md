# OlympusPipeliner: RISC-V Pipelined Processor Simulator

## Project Overview
OlympusPipeliner is a comprehensive RISC-V pipeline processor simulator that offers both forwarding and non-forwarding modes. This educational tool visualizes pipeline execution stages and data hazards, helping students understand the intricacies of modern processor design.

## Architecture Design

### Pipeline Structure
The processor implements the classic 5-stage RISC-V pipeline:
- **IF (Instruction Fetch)**: Retrieves instructions from memory
- **ID (Instruction Decode)**: Decodes instructions, reads registers
- **EX (Execute)**: Performs ALU operations, address calculations
- **MEM (Memory Access)**: Reads/writes data memory
- **WB (Write Back)**: Writes results back to registers

### Core Components
- **Register File**: 32 general-purpose registers (x0-x31)
- **Memory System**: Separate instruction and data memory
- **Pipeline Registers**: IF/ID, ID/EX, EX/MEM, MEM/WB
- **Control Logic**: Hazard detection, forwarding logic
- **Visualization**: Pipeline execution diagram generator

## Design Decisions

### 1. Early Branch Resolution
- Branch targets are calculated in the **ID stage** rather than EX
- Benefits:
  - Reduces branch penalties from 3 cycles to 1 cycle
  - Minimizes pipeline flushes
  - Branch conditions evaluated with freshly read register values
- Implementation details:
  - `handleBranchAndJump()` function processes branches in ID stage
  - Flushes only the IF stage upon branch taken

### 2. Register Usage Tracking
- Implemented an advanced register usage tracking system:
  - Uses a vector of vectors (`regUsageTracker`) for precise dependency tracking
  - Each register has a list of instructions currently using it
  - Register dependencies correctly modeled with instruction lifecycle
- Benefits:
  - More precise hazard detection compared to simple flags
  - Supports multiple in-flight instructions using the same register

### 3. 3D Pipeline Visualization
- Traditional pipeline diagrams show only one stage per instruction per cycle
- Our implementation uses a 3D matrix representation:
  - Multiple pipeline stages can be active for the same instruction in one cycle
  - Shows advanced behaviors like stalls and flushes
- Output format:
  - CSV file with cycle-by-cycle pipeline state
  - Multiple stages per cell separated by slashes when needed

### 4. Data Forwarding Implementation
- The `ForwardingProcessor` class extends the base processor to implement forwarding:
  - Detects data dependencies between instructions
  - Forwards results directly from EX/MEM and MEM/WB stages
  - Minimizes stalls due to data hazards
- Forwarding sources:
  - EX/MEM stage: Most recent result, highest priority
  - MEM/WB stage: Second priority if needed

### 5. Specialized Memory Access
- Support for various memory access types:
  - Byte (LB/SB), Half-word (LH/SH), Word (LW/SW)
  - Both signed and unsigned loads (LB/LBU, LH/LHU)
- Implementation details:
  - Memory class with specialized read/write methods for each size
  - Proper sign extension handling for signed loads

### 6. Load-Use Hazard Handling
- Even with forwarding, load-use hazards require special handling:
  - Data from memory load isn't available until MEM stage
  - Stall inserted when load result needed by next instruction
- Implementation:
  - Special detection condition in ID stage
  - Pipeline stalled for exactly one cycle

### 7. Immediate Extraction Logic
- RISC-V has multiple immediate encoding formats
- Our implementation properly extracts and sign-extends immediates for:
  - I-type, S-type, B-type, U-type, and J-type instructions
- Supports the full range of immediate values with correct sign extension

## Implementation Challenges

### 1. Correct Branch Handling
- **Challenge**: Moving branch resolution to ID stage required careful coordination
- **Solution**: 
  - Implemented specialized forwarding for branch operands in ID stage
  - Used the `evaluateBranchCondition()` function to handle all branch types
  - Advanced tracking of branch decisions and pipeline flushes

### 2. Data Hazard Detection
- **Challenge**: Identifying all possible data hazards accurately
- **Solution**:
  In non-forwarding mode: conservative approach using register usage tracking
  - In forwarding mode: precise identification of dependencies between instructions
  - Special handling for load-use hazards that can't be solved with forwarding -

### 3. Pipeline Visualization
- **Challenge**: Representing complex pipeline states clearly
- **Solution**:
  - 3D matrix structure allowing multiple stages per cycle
  - Custom print formats showing stalls and simultaneous stages
  - CSV output for consistent and readable pipeline diagrams

### 4. Forwarding Logic Implementation
- **Challenge**: Correctly identifying and implementing all forwarding paths
- **Solution**:
  - Two-level priority system for forwarding sources
  - Special handling for store instructions (forwarding to the stored value)
  - Proper handling of ALU sources for different instruction types

### 5. JALR Instruction Edge Cases
- **Challenge**: JALR combines register values with immediates and needs special handling
- **Solution**:
  - Early calculation of return address (PC+4) in ID stage
  - Proper forwarding for the source register value
  - Correct target address calculation with LSB clearing per spec

### 6. Testing and Debugging
- **Challenge**: Ensuring correct execution across various instruction sequences
- **Solution**:
  - Detailed cycle-by-cycle output showing all pipeline stages
  - Register and memory state tracking
  - Comprehensive test programs for different scenarios



### Building the Project
```

## Sources Used
- RISC-V Specifications: https://riscv.org/specifications/
- Computer Organization and Design RISC-V Edition: The Hardware Software Interface by David A. Patterson and John L. Hennessy
- Various online resources and academic papers on pipeline processor design

## Known Issues
- There is no branch predictor as told in the assignment pdf so might not be as efficient in pipelining as actual processors.