# OlympusPipeliner: RISC-V Pipelined Processor Simulator

## Project Overview
OlympusPipeliner is a comprehensive RISC-V pipeline processor simulator that offers both forwarding and non-forwarding modes.

## Architecture Design

### Core Components
- *Register File*: 32 general-purpose registers (x0-x31)
- *Memory System*: Separate instruction and data memory
- *Pipeline Registers*: IF/ID, ID/EX, EX/MEM, MEM/WB
- *Control Logic*: Hazard detection, forwarding logic
- *Visualization*: Pipeline execution diagram generator

## Design Decisions

### 1. Register and Memory 

We have designed such that the register and memory can take input values and all have been intialized to 0


### 2. Early Branch Resolution
- Branch targets are calculated in the *ID stage* rather than EX
- Benefits:
  - Reduces branch penalties reduce by one 
  - Minimizes pipeline flushes
  - Branch conditions evaluated with freshly read register values
- Implementation details:
  - handleBranchAndJump() function processes branches in ID stage
  - Flushes only the IF stage upon branch taken

### 3. Register Usage Tracking
- Implemented an advanced register usage tracking system:
  - Uses a vector of vectors (regUsageTracker) for precise dependency tracking
  - Each register has a list of instructions currently using it
  - Register dependencies correctly modeled with instruction lifecycle
- Benefits:
  - More precise hazard detection compared to simple flags
  - Supports multiple in-flight instructions using the same register

### 4. 3D Pipeline Visualization
- Traditional pipeline diagrams show only one stage per instruction per cycle

- Our implementation uses a 3D matrix representation
- Multiple pipeline stages can be active for the same instruction in one cycle resulting in the printing of Multiple stages per cell separated by slashes

- Output format:
  - We have added functionality to produce a CSV file with cycle-by-cycle pipeline state for an aesthetic look, but default is to produce a .txt file matching the format of autograder

  

### 5. Data Forwarding Implementation
- The ForwardingProcessor class extends the base processor to implement forwarding:
  - Detects data dependencies between instructions
  - Forwards results directly from EX/MEM and MEM/WB stages
  - Minimizes stalls due to data hazards

- All the functions from the non-forwarding implementation was reused except the run function which was overriden to implement forwarding logic by writing to register in that stage itself(EX or MEM) and then freeing it except for when the instr in branches which is updated only in the next cycle


### 6. Specialized Memory Access
- Support for various memory access types:
  - Byte (LB/SB), Half-word (LH/SH), Word (LW/SW)
  - Both signed and unsigned loads (LB/LBU, LH/LHU)
- Implementation details:
  - Memory class with specialized read/write methods for each size
  - Proper sign extension handling for signed loads

### 7. Load-Use Hazard Handling
- Even with forwarding, load-use hazards require special handling:
  - Data from memory load isn't available until MEM stage
  - Stall inserted when load result needed by next instruction

### 8. Immediate Extraction Logic
- RISC-V has multiple immediate encoding formats
- Our implementation properly extracts and sign-extends immediates for:
  - I-type, S-type, B-type, U-type, and J-type instructions
- Supports the full range of immediate values with correct sign extension

### 9. Unsigned and signed int usage
- The register can store negative values so it is signed 
- The register memory address is only non negative values as address is non negative so we have used unsigned to handle more range of values in the same amount of memory.

### 10. Processing of Instructions cycle-by-cycle

By handling the instructions one cycle after another such as first updating the WB latches and functions of WB before moving on to MEM stage helps us in easy extension of non-forwarding to forwarding processor(only an extra 20 lines of code)




## Implementation Challenges

### 1. Correct Branch Handling
- *Challenge*: Moving branch resolution to ID stage required careful coordination
- *Solution*: 
  - Implemented specialized forwarding for branch operands in ID stage
  - Used the evaluateBranchCondition() function to handle all branch types
  - Advanced tracking of branch decisions and pipeline flushes

### 2. Data Hazard Detection
- *Challenge*: Identifying all possible data hazards accurately
- *Solution*:
  In non-forwarding mode: conservative approach using register usage tracking
  - In forwarding mode: precise identification of dependencies between instructions
  - Special handling for load-use hazards that can't be solved with forwarding and also branch instructions that were dependent on the previous instruction

### 3. Pipeline Visualization
- *Challenge*: Representing complex pipeline states clearly
- *Solution*:
  - 3D matrix structure allowing multiple stages per cycle
  - Custom print formats showing stalls and simultaneous stages
  - CSV output for consistent and readable pipeline diagrams

### 4. Forwarding Logic Implementation
- *Challenge*: Correctly identifying and implementing all forwarding paths
- *Solution*:
  - Two-level priority system for forwarding sources
  - Special handling for store instructions (forwarding to the stored value)
  - Proper handling of ALU sources for different instruction types

### 5. JALR Instruction Edge Cases
- *Challenge*: JALR combines register values with immediates and needs special handling
- *Solution*:
  - Early calculation of return address (PC+4) in ID stage
  - Proper forwarding for the source register value
  - Correct target address calculation with LSB clearing per spec

### 6. Testing and Debugging
- *Challenge*: Ensuring correct execution across various instruction sequences
- *Solution*:
  - Used output format of .csv for clear visualisation and easy verification with peers and ripes
  - Detailed cycle-by-cycle output showing all pipeline stages
  - Register and memory state tracking
  - Comprehensive test programs for different scenarios

## Known Issues 

 No assumptions have been made and actual register processing has been simulated to mirror actual RIPES(actually better than ripes as ours does branch resolution in ID) and no Known Issues of now correctly giving output to all the examples(we took float as int in programs requiring it , as told in piazza post to not implement floating point calculation) mentioned in https://marz.utk.edu/my-courses/cosc230/book/example-risc-v-assembly-programs/

- la instruction was given in the examples website, but as that is a pseudo instruction we haven't supported it 



## Sources Used
- RISC-V Specifications: https://riscv.org/specifications/
- Computer Organization and Design RISC-V Edition: The Hardware Software Interface by David A. Patterson and John L. Hennessy
- Various online resources and academic papers on pipeline processor design
- Chatgpt for code regarding Machine code conversion to opcode,rs1,etc