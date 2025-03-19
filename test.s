.section .data
test_string:
    .string "Hello, RISC-V!"  # Test string to find the length of

.section .text
.global _start
_start:
    # Load the address of our test string into a0
    la      a0, test_string
    
    # Call our strlen function
    call    strlen
    
    # When we return, a0 will contain the length of the string
    # We would normally do something with this result
    # For simulator purposes, we can stop here
    
    # Exit the program
    li      a7, 10       # System call for exit
    ecall                # Make system call

.global strlen
strlen:
    # a0 = const char *str
    li     t0, 0         # i = 0
1: # Start of for loop
    add    t1, t0, a0    # Add the byte offset for str[i]
    lb     t1, 0(t1)     # Dereference str[i]
    beqz   t1, 1f        # if str[i] == 0, break for loop
    addi   t0, t0, 1     # Add 1 to our iterator
    j      1b            # Jump back to condition (1 backwards)
1: # End of for loop
    mv     a0, t0        # Move t0 into a0 to return
    ret                  # Return back via the return address register
