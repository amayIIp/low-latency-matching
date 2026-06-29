
import re

import os

def strip_cpp_comments(file_content):

    pattern = r'(\"(?:\\\\|\\\"|[^\"])*\")|(?://[^\n]*|/\*(?:[^*]|\*(?!/))*\*/)'

    def replace_match(match):

        matched_text = match.group(0)

        if matched_text.startswith('"'):
            return matched_text

        return ""

    cleaned_content = re.sub(pattern, replace_match, file_content)

    lines = cleaned_content.splitlines()
    cleaned_lines = [line.rstrip() for line in lines]

    non_empty_lines = []
    for line in cleaned_lines:

        if line.strip() != "":
            non_empty_lines.append(line)
        else:

            if not non_empty_lines or non_empty_lines[-1] != "":
                non_empty_lines.append("")

    return "\n".join(non_empty_lines) + "\n"

def strip_python_comments(file_content):

    pattern = r'(\"(?:\\\\|\\\"|[^\"])*\")|(\'(?:\\\\|\\\'|[^\'])*\')|#[^\n]*'

    def replace_match(match):
        matched_text = match.group(0)

        if matched_text.startswith('"') or matched_text.startswith("'"):
            return matched_text

        return ""

    cleaned_content = re.sub(pattern, replace_match, file_content)

    lines = cleaned_content.splitlines()
    cleaned_lines = [line.rstrip() for line in lines]

    non_empty_lines = []
    for line in cleaned_lines:
        if line.strip() != "":
            non_empty_lines.append(line)
        else:
            if not non_empty_lines or non_empty_lines[-1] != "":
                non_empty_lines.append("")

    return "\n".join(non_empty_lines) + "\n"

def main():

    root_dir = "/mnt/d/Low Latency Matching Engine"

    for dirpath, _, filenames in os.walk(root_dir):

        if "build" in dirpath or ".git" in dirpath or "tools" in dirpath:
            continue

        for filename in filenames:

            file_path = os.path.join(dirpath, filename)

            if filename.endswith(".hpp") or filename.endswith(".cpp"):

                with open(file_path, "r", encoding="utf-8") as f:
                    content = f.read()

                clean_content = strip_cpp_comments(content)

                with open(file_path, "w", encoding="utf-8") as f:
                    f.write(clean_content)
                print(f"Cleaned comments from: {file_path}")

            elif filename.endswith(".py"):
                with open(file_path, "r", encoding="utf-8") as f:
                    content = f.read()
                clean_content = strip_python_comments(content)
                with open(file_path, "w", encoding="utf-8") as f:
                    f.write(clean_content)
                print(f"Cleaned comments from: {file_path}")

if __name__ == "__main__":
    main()
