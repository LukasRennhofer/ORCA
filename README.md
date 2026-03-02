# ORCA

ORCA is an experimental programming language and virtual machine designed for exploring compiler architecture and VM design in a small, hackable environment.

It combines:

- A Java-style bytecode virtual machine

- A Rust-inspired type-safe system

- Controlled low-level access

- A minimal and understandable architecture

ORCA is designed to stay small enough to understand completely while still being powerful enough to build real software.

## Philosophy

Most languages grow into huge ecosystems that are impossible to fully understand.

ORCA goes the opposite direction.

The goal is a language where you can:

- Understand the compiler

- Understand the VM

- Modify the system

- Extend the language

- Keep control over execution

## Architecture

ORCA consists of only two core components:

- Compiler — Generates bytecode
- Virtual Machine — Executes bytecode

Both are implemented as single-header libraries, making ORCA easy to integrate into other projects.

## Why Use ORCA?

ORCA is already capable of solving real problems and running production code, although development is still ongoing.

It is currently used in my real projects and continues to evolve alongside them.

## Name

### ORCA = Open Ridiculous Compilation Architecture

Because every programming language deserves an unnecessarily dramatic acronym.