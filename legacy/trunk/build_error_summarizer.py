#!/usr/bin/env python3
"""
Build Error Summarizer

Parses GCC/Clang build logs and summarizes errors by file, type, and line.
Reduces token usage for AI analysis by providing structured error data.

Usage: python3 build_error_summarizer.py < build.log
Or: make 2>&1 | python3 build_error_summarizer.py
"""

import sys
import re
from collections import defaultdict, Counter

def parse_gcc_error(line):
    """Parse GCC/Clang error/warning line."""
    # Pattern: file:line:col: error|warning: message
    pattern = r'^([^:]+):(\d+):(\d+):\s*(error|warning|note):\s*(.+)$'
    match = re.match(pattern, line.strip())
    if match:
        file_path, line_num, col, error_type, message = match.groups()
        return {
            'file': file_path,
            'line': int(line_num),
            'col': int(col),
            'type': error_type,
            'message': message.strip()
        }
    return None

def main():
    errors = []
    current_error = None

    # Read all input
    content = sys.stdin.read()
    
    # Find all GCC error lines
    import re
    error_pattern = r'^([^:]+):(\d+):(\d+):\s*(error|warning|note):\s*(.+)$'
    lines = content.splitlines() if '\n' in content else [content]
    
    for line in lines:
        line = line.strip()
        if not line:
            continue

        parsed = parse_gcc_error(line)
        if parsed:
            if current_error:
                errors.append(current_error)
            current_error = parsed
        elif current_error:
            # Continuation of previous error (e.g., notes)
            if line.startswith('   ') or line.startswith('      '):
                current_error['message'] += '\n' + line.strip()
            else:
                # New unrelated line, save previous
                errors.append(current_error)
                current_error = None

    if current_error:
        errors.append(current_error)

    # Summarize
    if not errors:
        print("No errors found in input.")
        return

    # Group by file
    by_file = defaultdict(list)
    for err in errors:
        by_file[err['file']].append(err)

    # Count error types
    type_counts = Counter(err['type'] for err in errors)

    print("=== Build Error Summary ===")
    print(f"Total errors/warnings: {len(errors)}")
    print(f"Error types: {dict(type_counts)}")
    print(f"Files affected: {len(by_file)}")
    print()

    for file_path, file_errors in sorted(by_file.items()):
        print(f"File: {file_path} ({len(file_errors)} issues)")

        # Group by message
        msg_groups = defaultdict(list)
        for err in file_errors:
            key = (err['type'], err['message'])
            msg_groups[key].append(err['line'])

        for (err_type, message), lines in sorted(msg_groups.items()):
            lines_str = ', '.join(map(str, sorted(set(lines))))  # Unique, sorted lines
            print(f"  {err_type}: {message[:100]}{'...' if len(message) > 100 else ''}")
            print(f"    Lines: {lines_str}")
        print()

    # Most common error patterns
    messages = [err['message'].split('\n')[0] for err in errors]  # First line only
    common = Counter(messages).most_common(5)
    if common:
        print("=== Most Common Error Patterns ===")
        for msg, count in common:
            print(f"{count}x: {msg[:120]}{'...' if len(msg) > 120 else ''}")

if __name__ == '__main__':
    main()