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

#include <vector>

#include "orca_common.hpp"

#include <iostream>
#include <stdexcept>

namespace orca {

    enum class OrcaType {
        INT,
        LONG,
        FLOAT,
        DOUBLE,
        CHAR,
        SHORT,
        BYTE,
        BOOL // This is represented as an INT in the VM
    };

    struct OrcaValue {
        OrcaType type;
        union { 
            OrcaInt    d_integer; 
            OrcaLong   d_long; 
            OrcaFloat  d_float; 
            OrcaDouble d_double;
            OrcaChar   d_char;
            OrcaShort  d_short;
            OrcaByte   d_byte;
        } data;
    };

    struct StackFrame {
        std::vector<OrcaValue> stack;
        std::vector<OrcaValue> var_pool;
    };

    // The Virtual Machine class
    class VM {

        public:
            VM() : m_pc(0) {}
            
            void runBinary(const std::vector<uint8_t>& bytecode) {
                // Validate magic number and version
                if (bytecode.size() < 6) {
                    throw std::runtime_error("Invalid bytecode: too short");
                }

                uint32_t magic = bytecode[0] | (bytecode[1] << 8) | (bytecode[2] << 16) | (bytecode[3] << 24);
                if (magic != BYTECODE_MAGIC) {
                    throw std::runtime_error("Invalid bytecode: bad magic number");
                }

                uint16_t version = bytecode[4] | (bytecode[5] << 8);
                if (version != BYTECODE_VERSION) {
                    throw std::runtime_error("Invalid bytecode: unsupported version");
                }

                m_pc = 6; // Start after magic number and version

                while (m_pc < bytecode.size()) {
                    uint8_t op = bytecode[m_pc++];

                    auto readU16 = [&bytecode, this]() -> uint16_t {
                        if (m_pc + 2 > bytecode.size()) return 0;
                        uint16_t v = bytecode[m_pc] | (bytecode[m_pc+1] << 8);
                        m_pc += 2;
                        return v;
                    };

                    switch (static_cast<Opcode>(op)) {

                        case Opcode::POP: {
                            if (!m_stackframe.stack.empty()) {
                                m_stackframe.stack.pop_back();
                            }
                            break;
                        }

                        case Opcode::DUP: {
                            if (!m_stackframe.stack.empty()) {
                                m_stackframe.stack.push_back(m_stackframe.stack.back());
                            }
                            break;
                        }

                        case Opcode::ICONST: {
                            // read 4-byte int operand
                            if (m_pc + 4 > bytecode.size()) break;;
                            OrcaInt value = *reinterpret_cast<const OrcaInt*>(&bytecode[m_pc]);
                            m_pc += 4;

                            OrcaValue c;
                            c.data.d_integer = value;
                            c.type = OrcaType::INT;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        case Opcode::ILOAD: {
                            uint16_t index = readU16();
                            m_stackframe.stack.push_back(m_stackframe.var_pool[index]);
                            break;
                        }

                        case Opcode::ISTORE: {
                            uint16_t index = readU16();
                            if (index >= m_stackframe.var_pool.size()) {
                                m_stackframe.var_pool.resize(index + 1);
                            }
                            m_stackframe.var_pool[index] = m_stackframe.stack.back();
                            m_stackframe.stack.pop_back();
                            break;
                        }

                        case Opcode::TRUNCVARS: {
                            uint16_t newSize = readU16();
                            if (newSize > m_stackframe.var_pool.size()) {
                                throw std::runtime_error("Invalid TRUNCVARS operand");
                            }
                            m_stackframe.var_pool.resize(newSize);
                            break;
                        }

                        case Opcode::JUMP: {
                            uint16_t target = readU16();
                            if (target > bytecode.size()) {
                                throw std::runtime_error("Invalid jump target");
                            }
                            m_pc = target;
                            break;
                        }

                        case Opcode::JUMP_IF_FALSE: {
                            uint16_t target = readU16();
                            if (m_stackframe.stack.empty()) {
                                throw std::runtime_error("Empty stack for JUMP_IF_FALSE");
                            }

                            OrcaInt condition = m_stackframe.stack.back().data.d_integer;
                            m_stackframe.stack.pop_back();

                            if (condition == 0) {
                                if (target > bytecode.size()) {
                                    throw std::runtime_error("Invalid jump target");
                                }
                                m_pc = target;
                            }
                            break;
                        }

                        case Opcode::IADD: {
                            OrcaInt b = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();
                            OrcaInt a = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();

                            OrcaValue c;
                            c.data.d_integer = a + b;
                            c.type = OrcaType::INT;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        case Opcode::ISUB: {
                            OrcaInt b = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();
                            OrcaInt a = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();

                            OrcaValue c;
                            c.data.d_integer = a - b;
                            c.type = OrcaType::INT;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        case Opcode::IMUL: {
                            OrcaInt b = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();
                            OrcaInt a = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();

                            OrcaValue c;
                            c.data.d_integer = a * b;
                            c.type = OrcaType::INT;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        case Opcode::IDIV: {
                            OrcaInt b = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();
                            OrcaInt a = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();

                            if (b == 0) {
                                throw std::runtime_error("Division by zero");
                            }

                            OrcaValue c;
                            c.data.d_integer = a / b;
                            c.type = OrcaType::INT;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        case Opcode::IAND: {
                            OrcaInt b = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();
                            OrcaInt a = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();

                            OrcaValue c;
                            c.data.d_integer = a & b;
                            c.type = OrcaType::INT;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        case Opcode::IOR: {
                            OrcaInt b = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();
                            OrcaInt a = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();

                            OrcaValue c;
                            c.data.d_integer = a | b;
                            c.type = OrcaType::INT;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        case Opcode::IXOR: {
                            OrcaInt b = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();
                            OrcaInt a = m_stackframe.stack.back().data.d_integer; m_stackframe.stack.pop_back();

                            OrcaValue c;
                            c.data.d_integer = a ^ b;
                            c.type = OrcaType::INT;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        // Long operations
                        case Opcode::LCONST: {
                            if (m_pc + 8 > bytecode.size()) break;
                            OrcaLong value = *reinterpret_cast<const OrcaLong*>(&bytecode[m_pc]);
                            m_pc += 8;

                            OrcaValue c;
                            c.data.d_long = value;
                            c.type = OrcaType::LONG;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        case Opcode::LLOAD: {
                            uint16_t index = readU16();
                            m_stackframe.stack.push_back(m_stackframe.var_pool[index]);
                            break;
                        }

                        case Opcode::LSTORE: {
                            uint16_t index = readU16();
                            if (index >= m_stackframe.var_pool.size()) {
                                m_stackframe.var_pool.resize(index + 1);
                            }
                            m_stackframe.var_pool[index] = m_stackframe.stack.back();
                            m_stackframe.stack.pop_back();
                            break;
                        }

                        case Opcode::LADD: {
                            OrcaLong b = m_stackframe.stack.back().data.d_long; m_stackframe.stack.pop_back();
                            OrcaLong a = m_stackframe.stack.back().data.d_long; m_stackframe.stack.pop_back();

                            OrcaValue c;
                            c.data.d_long = a + b;
                            c.type = OrcaType::LONG;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        case Opcode::LSUB: {
                            OrcaLong b = m_stackframe.stack.back().data.d_long; m_stackframe.stack.pop_back();
                            OrcaLong a = m_stackframe.stack.back().data.d_long; m_stackframe.stack.pop_back();

                            OrcaValue c;
                            c.data.d_long = a - b;
                            c.type = OrcaType::LONG;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        case Opcode::LMUL: {
                            OrcaLong b = m_stackframe.stack.back().data.d_long; m_stackframe.stack.pop_back();
                            OrcaLong a = m_stackframe.stack.back().data.d_long; m_stackframe.stack.pop_back();

                            OrcaValue c;
                            c.data.d_long = a * b;
                            c.type = OrcaType::LONG;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        case Opcode::LDIV: {
                            OrcaLong b = m_stackframe.stack.back().data.d_long; m_stackframe.stack.pop_back();
                            OrcaLong a = m_stackframe.stack.back().data.d_long; m_stackframe.stack.pop_back();

                            if (b == 0) {
                                throw std::runtime_error("Division by zero");
                            }

                            OrcaValue c;
                            c.data.d_long = a / b;
                            c.type = OrcaType::LONG;

                            m_stackframe.stack.push_back(c);
                            break;
                        }

                        // TODO: Move away, just for test now
                        case Opcode::DEBUG:
                            std::cout << "=== DEBUG ===" << std::endl;
                            if (!m_stackframe.stack.empty()) {
                                auto& val = m_stackframe.stack.back();
                                std::cout << "  Stack top: ";
                                if (val.type == OrcaType::LONG) {
                                    std::cout << val.data.d_long << "L";
                                } else {
                                    std::cout << val.data.d_integer;
                                }
                                std::cout << std::endl;
                            } else {
                                std::cout << "  Stack: empty" << std::endl;
                            }
                            
                            std::cout << "  Variables (" << m_stackframe.var_pool.size() << "):" << std::endl;
                            for (size_t i = 0; i < m_stackframe.var_pool.size(); ++i) {
                                std::cout << "    [" << i << "] = ";
                                if (m_stackframe.var_pool[i].type == OrcaType::LONG) {
                                    std::cout << m_stackframe.var_pool[i].data.d_long << "L";
                                } else {
                                    std::cout << m_stackframe.var_pool[i].data.d_integer;
                                }
                                std::cout << std::endl;
                            }
                            break;

                        default:
                            throw std::runtime_error("Unknown opcode in bytecode");
                    }
                }
            }
        private:
            StackFrame m_stackframe;

            // General Purpose
            size_t m_pc; // Program Counter
    };

}