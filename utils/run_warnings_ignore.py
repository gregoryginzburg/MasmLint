import yaml
import os
from pathlib import Path

# Ensure CWD is set to its own directory
script_dir = os.path.dirname(os.path.realpath(__file__))
os.chdir(script_dir)

import shutil
import os
import tempfile

def safe_write(file_path: Path, new_lines):
    # Create a backup
    backup_path = file_path.parent / (file_path.name + '.bak')
    shutil.copyfile(file_path, backup_path)

    try:
        # Write to a temporary file
        with tempfile.NamedTemporaryFile('w', delete=False) as temp_file:
            temp_file.writelines(new_lines)
            temp_path = temp_file.name

        # Replace the original file with the modified file
        shutil.move(temp_path, file_path)

    except Exception as e:
        # In case of an error, restore from backup
        shutil.move(backup_path, file_path)
        raise e
    finally:
        # Clean up: remove backup file if it exists
        if os.path.exists(backup_path):
            os.remove(backup_path)


def read_config(file_path: str):
    with open(file_path, 'r') as file:
        config = yaml.safe_load(file)
    return config


def apply_to_files(src_path: str, config: dict):
    path = Path(src_path)

    for exclude_path_settings in config:
        exclude_glob = exclude_path_settings['path']
        for file_path in path.glob(exclude_glob):
            apply_rule_to_file(file_path, exclude_path_settings)


def delete_previous_supressions(file_path):
    lines_filtered = None
    with open(file_path, 'r') as file:
        lines = list(file)
        line_indices_to_delete = set()
        for i, line in enumerate(lines):
            # cppcheck
            if line.startswith("// NOLINTBEGIN"):
                line_indices_to_delete.add(i)
                line_indices_to_delete.add(i - 1)
                break
                
        for i, line in enumerate(reversed(lines)):
            # cppcheck
            if line.startswith("// NOLINTEND"):
                line_indices_to_delete.add(i + len(lines) - 1)
                line_indices_to_delete.add(i + len(lines) - 2)
                break

        lines_filtered = [line for i, line in enumerate(lines) if i not in line_indices_to_delete]
    
    safe_write(file_path, lines_filtered)

        

def apply_rule_to_file(file_path: Path, rules: dict):
    top_strings = []
    bottom_strings = []
    # First delete old automatically generated suppressions
    delete_previous_supressions(file_path)
    
    # Then proceed to add new suppressions
    # clang-tidy - add //NOLINT blocks to the file
    # // NOLINTBEGIN(google-explicit-constructor, google-runtime-int)
    # // NOLINTEND(...)
    if rules.get('clang-tidy') is not None:
        clang_tidy_options = rules['clang-tidy']
        if clang_tidy_options == 'all':
            disabled_warnings_str = ""
        else:
            disabled_warnings_str = f"({','.join(clang_tidy_options)})"
        top_string = [f"// NOTE: Automatically generated by 'run_warnings_ignore.py'\n",
                      f"// NOLINTBEGIN{disabled_warnings_str}\n"]
        bottom_string = [f"// NOTE: Automatically generated by 'run_warnings_ignore.py'\n",
                         f"// NOLINTEND{disabled_warnings_str}\n"]

        top_strings.extend(top_string)
        bottom_strings.extend(bottom_string)
    
    # cppcheck - create a .cppcheck_ignore file
    if rules.get('cppcheck') is not None:
        cppcheck_options = rules['cppcheck']
        # TODO: implement convertation (does cppcheck accept relative paths?)
        # and also change the cmake too
        

    # compiler warnings
    # TODO: ???
    
    # Write added lines to the file
    lines = None
    with open(file_path, 'r') as file:
        lines = list(file)
    
    if lines[-1].endswith('\n'):
        new_lines = top_strings + lines + bottom_strings
    else:
        new_lines = top_strings + lines + ["\n"] + bottom_strings
    
    safe_write(file_path, new_lines)
        

def main():
    config_path = '../.warnings_ignore.yml'
    config = read_config(config_path)

    apply_to_files('../', config)


if __name__ == "__main__":
    main()
