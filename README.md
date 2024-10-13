[![ci](https://github.com/gregoryginzburg/MasmLint/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/gregoryginzburg/MasmLint/actions/workflows/ci-windows.yml)
[![ci](https://github.com/gregoryginzburg/MasmLint/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/gregoryginzburg/MasmLint/actions/workflows/ci-linux.yml)
<!-- [![Coverage Status](https://coveralls.io/repos/github/gregoryginzburg/MasmLint/badge.svg?branch=main)](https://coveralls.io/github/gregoryginzburg/MasmLint?branch=main) -->


# MasmLint - Linter for MASM Assembly Code

A lightweight linter for detecting common issues in MASM (Microsoft Macro Assembler) code.

## Features

- Identifies common mistakes in MASM assembly code

## Installation

1. Clone the repository:
    ```bash
    git clone https://github.com/yourusername/MasmLint
    ```
2. Navigate to the project directory:
    ```bash
    cd MasmLint
    ```
3. Generate the build system using CMake:
    ```bash
    cmake -S . -B build -G "Ninja Multi-Config"  # For Ninja
    # OR
    cmake -S . -B build -G "Visual Studio 17 2022"  # For Visual Studio
    # OR
    cmake -S . -B build -G "Unix Makefiles"  # For Makefile
    ```
4. Build the project:
    ```bash
    cmake --build build
    ```

## Requirements

- GCC or Clang or MSVC
- CMake
- Ninja, Visual Studio, or Makefile (depending on your preferred build system)

## Dependencies

- doctest (for unit testing)
- spdlog (for logging)

## Tooling

### Compilers

- GCC (GNU Compiler Collection)
- Clang
- MSVC (Microsoft Visual C++)

### Build System

- CMake (Supports Ninja, Visual Studio, Makefile)

### Static Analysis

- clang-tidy
- cppcheck

### Dynamic Analysis

- AddressSanitizer
- ThreadSanitizer
- UndefinedBehaviorSanitizer
- MemorySanitizer

### Code Formatting

- clang-format

### Unit Testing

- doctest

### CI/CD

- GitHub Actions

### Code Coverage

- codecov
- gcov (llvm-cov)

---

### License

This project is licensed under the MIT License.

---
