[![ci](https://github.com/gregoryginzburg/RealSky/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/gregoryginzburg/RealSky/actions/workflows/ci-windows.yml)
[![ci](https://github.com/gregoryginzburg/RealSky/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/gregoryginzburg/RealSky/actions/workflows/ci-linux.yml)

# Realistic Sky Generator for Blender

A Blender addon written in C++ for generating realistic skies.

## Features

- Generates realistic skies with dynamic lighting
- Customizable sky parameters such as cloud density, sun position, etc.

## Installation

1. Clone the repository
    ```bash
    git clone https://github.com/gregoryginzburg/RealSky
    ```
2. Navigate to the project directory and create a build folder
    ```bash
    cd RealSky
    ```
3. Generate the build system using CMake
    ```bash
    cmake -S . -B build -G "Ninja Multi-Config"  # For Ninja
    # OR
    cmake -S . -B build -G "Visual Studio 17 2022"  # For Visual Studio
    # OR
    cmake -S . -B build -G "Unix Makefiles"  # For Makefile
    ```
4. Build the project
    ```bash
    cmake --build build
    ```

## Requirements

- GCC or Clang or MSVC
- CMake
- Ninja, Visual Studio, or Makefile (depending on your build system)

## Dependencies
- imgui
- glew
- glfw
- glm
- doctest
- spdlog


## Tooling

### Compilers

- GCC (GNU Compiler Collection)
- Clang
- MSVC (Microsoft Visual C++)

### Build System

- CMake (Generates for Ninja, Visual Studio, Makefile)

### Version Control

- Git

### Debuggers

- GDB (GNU Debugger)
- Microsoft Visual Studio Debugger

### Static Analysis

- clang-tidy
- cppcheck

### Dynamic Analysis

- AddressSanitizer
- ThreadSanitizer
- UndefinedBehaviorSanitizer
- MemorySanitizer
- Cuda Sanitizer

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

_later_ **Fuzz Testing:** ?

_later_ **Mutation Testing:** ?

_later_ **Documentation:**

- Doxygen (hdoc?)

_later_ **Licensing:**

- spdf

_later_ **Profiling Tools**:

- gprof
- perf
- Intel VTune Amplifier
- Valgrind's Callgrind and Cachegrind
- VerySleepy
- Nsight Compute
- more?

_later_ **Benchmarking:**

- google benchmark?

_later_ **Extra Tools:**

- [https://learn.microsoft.com/en-us/sysinternals/downloads/vmmap](https://learn.microsoft.com/en-us/sysinternals/downloads/vmmap "smartCard-inline")
- [https://learn.microsoft.com/en-us/sysinternals/downloads/process-explorer](https://learn.microsoft.com/en-us/sysinternals/downloads/process-explorer "smartCard-inline")
- more?

_later_ **Ship with hardening enabled:**

- Control Flow Guard - [https://learn.microsoft.com/en-us/cpp/build/reference/guard-enable-control-flow-guard?view=msvc-170](https://learn.microsoft.com/en-us/cpp/build/reference/guard-enable-control-flow-guard?view=msvc-170 "smartCard-inline")
- \_FORITFY\_SOURCE - [https://developers.redhat.com/articles/2022/09/17/gccs-new-fortification-level](https://developers.redhat.com/articles/2022/09/17/gccs-new-fortification-level "smartCard-inline")
- Stack Protector - [https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html "smartCard-inline")
- UBSan "Minimal runtime" mode - [https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html#minimal-runtime](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html#minimal-runtime "smartCard-inline")
