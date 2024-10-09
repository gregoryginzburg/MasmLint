import argparse
import os
import subprocess
import sys
import platform
from pathlib import Path
import shutil

LINUX_COMPILERS = ["gcc", "clang"]
WINDOWS_COMPILERS = ["msvc", "clang"]
WINDOWS_GENERATORS = ["Visual Studio 17 2022", "Ninja Multi-Config"]
LINUX_GENERATORS = ["Ninja Multi-Config", "Unix Makefiles"]
BUILD_DIR = "build"
BUILD_TYPES = {
    "debug": "Debug",
    "release": "Release",
    "relwithdebinfo": "RelWithDebInfo"
}


def match_best_config_preset(args):
    if args.config_preset is not None:
        compiler, build_type = map(str.lower, args.config_preset.split("-"))

    operating_system = platform.system()
    default_compiler = None
    if operating_system == "Linux":
        default_compiler = LINUX_COMPILERS[0]
    elif operating_system == "Windows":
        default_compiler = WINDOWS_COMPILERS[0]

    if args.config_preset is None:
        compiler = args.compiler.lower() if args.compiler else default_compiler
        build_type = args.build_type.lower()

    if operating_system == "Linux" and compiler not in LINUX_COMPILERS:
        print(f"You can't compile with {compiler} on {operating_system}")
        exit(1)
    if operating_system == "Windows" and compiler not in WINDOWS_COMPILERS:
        print(f"You can't compile with {compiler} on {operating_system}")
        exit(1)

    config_preset = f"{operating_system.lower()}-{compiler.lower()}"
    return config_preset


def match_best_build_preset(args):
    build_type = args.build_type.lower()
    config_preset = match_best_config_preset(args)
    build_preset = f"{config_preset}-{build_type}"
    return build_preset


def delete_cmake_cache():
    cache_path = Path(f"{BUILD_DIR}/CMakeCache.txt")
    cache_path.unlink(missing_ok=True)


def configure_project(args):
    config_preset = match_best_config_preset(args)

    configure_command = f"cmake -S . -B {BUILD_DIR} --preset {config_preset}"

    if args.no_warnings:
        configure_command += " -DML_ENABLE_COMPILER_WARNINGS:BOOL=FALSE -DML_ENABLE_CLANG_TIDY:BOOL=FALSE -DML_ENABLE_CPPCHECK:BOOL=FALSE"

    return [configure_command]


def compile_project(args, configure_first=False):
    compile_command = []
    if configure_first:
        compile_command.extend(configure_project(args))

    build_preset = match_best_build_preset(args)
    compile_command += [f"cmake --build {BUILD_DIR} --preset {build_preset}"]
    return compile_command


def run_program(args):
    build_type = args.build_type.lower()
    if build_type == "debug":
        base_name = "masm_dbg"
    else:
        base_name = "masm"

    if platform.system() == "Linux":
        run_command = f"cd bin && ./{base_name}"
    elif platform.system() == "Windows":
        run_command = f"cd bin && .\\{base_name}.exe"
    return [run_command]


def test_program(args):
    build_type = BUILD_TYPES.get(args.build_type.lower())
    test_command = f"ctest -C {build_type} --test-dir {BUILD_DIR}"
    return [test_command]


def execute_command(command, cwd=None):
    result = subprocess.run(command, shell=True, cwd=cwd)
    if result.returncode != 0:
        print(
            f"Command '{command}' failed with return code {result.returncode}")
        sys.exit(result.returncode)


def execute_commands(commands, cwd=None):
    for command in commands:
        execute_command(command)


def main():
    parser = argparse.ArgumentParser(
        description="Convenience wrapper for CMake commands.")
    parser.add_argument(
        "command", nargs='?', help="The command to execute (compile, run, clean, clean_cache, test, compile_run, start_runner, stop_runner).")
    parser.add_argument("-p", "--preset", dest="config_preset",
                        help="Specify the CMake preset")
    parser.add_argument("-c", "--compiler", help="Specify the compiler.")
    # parser.add_argument("--generator", help="Specify the generator.")
    parser.add_argument("-C", "--config", default="Debug",
                        dest="build_type",
                        help="Specify the build type (Release, Debug, RelWithDebInfo).")
    parser.add_argument("-W", "--no_warnings", action="store_true",
                        help="Compile without warnings.")
    args = parser.parse_args()

    # Default actions is to compile
    command = args.command if args.command else "compile"

    # Assert all arguments values are correct
    if args.compiler:
        assert (args.compiler.lower()
                in LINUX_COMPILERS or args.compiler.lower() in WINDOWS_COMPILERS)
    if args.build_type:
        assert (args.build_type.lower() in BUILD_TYPES.keys())

    if command == "configure":
        commands = configure_project(args)
        execute_commands(commands)
    elif command == "compile":
        commands = compile_project(args, configure_first=True)
        print(commands)
        execute_commands(commands)
    elif command == "run":
        commands = run_program(args)
        print(commands)
        execute_commands(commands)
    elif command == "compile_run":
        compile_commands = compile_project(args, configure_first=True)
        run_commands = run_program(args)
        print(commands)
        execute_commands(compile_commands)
        print('Compiling finished!')
        execute_commands(run_commands)
    elif command == "clean":
        shutil.rmtree(BUILD_DIR, ignore_errors=True)
        shutil.rmtree("bin", ignore_errors=True)
    elif command == "clean_cache":
        delete_cmake_cache()
    elif command == "test":
        commands = test_program(args)
        print(commands)
        execute_commands(commands)
    elif command == "start_runner":
        if platform.system() != "Windows":
            print("This command can only be run on Windows")
            sys.exit(1)
        sys.path[0] += "/.."
        from utils.self_hosted_runners import start_vm, start_windows_server, VM_NAME
        start_vm(VM_NAME)
        start_windows_server()
    elif command == "stop_runner":
        if platform.system() != "Windows":
            print("This command can only be run on Windows")
            sys.exit(1)
        sys.path[0] += "/.."
        from utils.self_hosted_runners import stop_vm, stop_windows_server, VM_NAME
        stop_windows_server()
        stop_vm(VM_NAME)
    else:
        print(f"Unknown command {args.command}")


if __name__ == "__main__":
    main()
