/*
    ORCA-Lang Virtual Machine / Compiler
    Open Ridiculous Compilation Architecture

    Copyright (C) 2026 Lukas Rennhofer

    This file is part of the ORCA project.

    ORCA is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3
    as published by the Free Software Foundation.

    ORCA is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    SPDX-License-Identifier: GPL-3.0
*/

#pragma once

#include "stdint.h"

namespace orca {

    // Magic number: "ORCA" in ASCII
    constexpr uint32_t BYTECODE_MAGIC = 0x4143524F; // 'O' 'R' 'C' 'A' (little-endian)
    constexpr uint16_t BYTECODE_VERSION = 1; // Version 1

    enum class Opcode : uint8_t {
        // Stack operations
        POP     = 0x00, // pops top of stack
        DUP     = 0x01, // duplicates top of stack

        // Integer (32-bit) operations
        ILOAD   = 0x02, // loads an integer from a local variable onto stack [operand: index]
        ISTORE  = 0x03, // stores an integer in a local variable [operand: index]
        IADD    = 0x04, // adds top two integers on the stack
        ISUB    = 0x05, // subtracts top two integers on the stack
        IMUL    = 0x06, // multiplies top two integers on the stack
        IDIV    = 0x07, // divides top two integers on the stack
        IAND    = 0x08, // Bitwise AND for two top integers on the stack
        IOR     = 0x09, // Bitwise OR for two top integers on the stack
        IXOR    = 0x0A, // Bitwise XOR for two top integers on the stack
        ICONST  = 0x0B, // push 32-bit integer constant onto stack [operand: 4 bytes]

        // Control flow
        JUMP    = 0x10, // unconditional jump [operand: target pc]
        JUMP_IF_FALSE = 0x11, // pops int condition and jumps when condition == 0 [operand: target pc]
        GOTO    = 0x12, // GOTO Statement to jump to a specific Opcode [operand: index]

        // System/Debug
        BREAKPOINT = 0x13, // Reserved for ORCA Debuggers
        TRUNCVARS = 0x14, // truncate local variable pool to size [operand: index]
        DEBUG   = 0x15, // print top of stack (temporary)
        
        // Long operations (64-bit integer)
        LLOAD   = 0x20, // load long from var pool [operand: index]
        LSTORE  = 0x21, // store long to var pool [operand: index]
        LADD    = 0x22, // add two longs
        LSUB    = 0x23, // subtract two longs
        LMUL    = 0x24, // multiply two longs
        LDIV    = 0x25, // divide two longs
        LCONST  = 0x26, // push 64-bit long constant [operand: 8 bytes]
        
        // Float operations (32-bit float)
        FLOAD   = 0x30, // load float from var pool [operand: index]
        FSTORE  = 0x31, // store float to var pool [operand: index]
        FADD    = 0x32, // add two floats
        FSUB    = 0x33, // subtract two floats
        FMUL    = 0x34, // multiply two floats
        FDIV    = 0x35, // divide two floats
        FCONST  = 0x36, // push 32-bit float constant [operand: 4 bytes]
        
        // Double operations (64-bit float)
        DLOAD   = 0x40, // load double from var pool [operand: index]
        DSTORE  = 0x41, // store double to var pool [operand: index]
        DADD    = 0x42, // add two doubles
        DSUB    = 0x43, // subtract two doubles
        DMUL    = 0x44, // multiply two doubles
        DDIV    = 0x45, // divide two doubles
        DCONST  = 0x46, // push 64-bit double constant [operand: 8 bytes]
    };

    typedef uint16_t OLocalVariableStorageSize;

    // Types
    typedef int32_t OrcaInt;
    typedef int64_t OrcaLong;
    typedef float    OrcaFloat;
    typedef double   OrcaDouble;
    typedef int16_t OrcaShort;
    typedef int16_t OrcaChar; // UTF-16
    typedef uint8_t  OrcaByte;

    // I handle Booleans as ints in the VM for the Int-Operations
}