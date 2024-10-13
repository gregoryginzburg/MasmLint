import os

def count_lines(filename):
    """Count the number of lines in a file."""
    with open(filename, 'r', encoding="utf-8", errors="ignore") as f:
        return len(f.readlines())

def total_lines_in_directory(directory='.'):
    """Recursively count lines in all .txt, .cpp, and .h files in the given directory and subdirectories."""
    total_lines = 0
    final_str = ""

    for root, subfolders, files in os.walk(directory):
        # print(root, subfolders, files)
        if 'build' in subfolders:
            subfolders.remove('build')
            subfolders.remove('.git')
            subfolders.remove('.github')
            subfolders.remove('.vscode')
            subfolders.remove('ext')
        for file in files:
            # file.endswith(('.txt', '.cpp', '.h', '.cmake'))
            if file.endswith(('.cpp', '.h')):
                file_path = os.path.join(root, file)
                try:
                    t = count_lines(file_path)
                    print(file_path, ": ", t)
                    total_lines += t
                    
                    # for gpt
                    final_str += f"File {file}:\n\n"
                    with open(file_path) as file:
                        final_str += file.read()
                    final_str += "\n"
                    
                except Exception as e:
                    print(f"Error reading {file_path}: {e}")

    return total_lines, final_str


if __name__ == "__main__":
    directory = os.getcwd()  # get current directory
    # directory += '\\..'
    total_lines, final_str = total_lines_in_directory(directory)
    with open("utils/full_code.txt", mode='w') as file:
        file.write(final_str)
    print(f"Total number of lines in .txt, .cpp, and .h files in '{directory}' and its subdirectories: {total_lines}")
