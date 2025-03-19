#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <assembly_file>"
    exit 1
fi

ASSEMBLY_FILE=$1
OBJ_FILE="${ASSEMBLY_FILE%.s}.o"
DUMP_FILE="${ASSEMBLY_FILE%.s}.dump"
# Change output path to place files directly in srcs directory
MC_FILE="srcs/$(basename ${ASSEMBLY_FILE%.s}).txt"

# Assemble the RISC-V code
riscv64-unknown-elf-as -march=rv32im -mabi=ilp32 $ASSEMBLY_FILE -o $OBJ_FILE

# Dump the object file to get machine code
riscv64-unknown-elf-objdump -d $OBJ_FILE > $DUMP_FILE

# Extract machine code and format for simulator - don't include header comment line
# that might be parsed as instructions
grep -A 100 "<_start>:" $DUMP_FILE | grep ":" | while read -r line; do
    ADDR=$(echo $line | awk '{print $1}' | sed 's/:$//')
    MC=$(echo $line | awk '{print $2}')
    if [ ! -z "$MC" ]; then
        echo "0x$ADDR 0x$MC" >> $MC_FILE
    fi
done

# Also extract the strlen function
grep -A 100 "<strlen>:" $DUMP_FILE | grep -v "<strlen>:" | grep ":" | while read -r line; do
    ADDR=$(echo $line | awk '{print $1}' | sed 's/:$//')
    MC=$(echo $line | awk '{print $2}')
    if [ ! -z "$MC" ]; then
        echo "0x$ADDR 0x$MC" >> $MC_FILE
    fi
done

# Extract data section if needed (string data)
grep -A 100 ".data" $DUMP_FILE | grep ":" | while read -r line; do
    ADDR=$(echo $line | awk '{print $1}' | sed 's/:$//')
    MC=$(echo $line | awk '{print $2}')
    if [ ! -z "$MC" ]; then
        echo "0x$ADDR 0x$MC" >> $MC_FILE
    fi
done

echo "Generated machine code in $MC_FILE"
