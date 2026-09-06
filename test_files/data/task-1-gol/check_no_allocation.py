#!/usr/bin/env python3
"""Reject common C allocation calls, then run the C++ allocation monitor."""
import re
import subprocess
import sys
from pathlib import Path

FORBIDDEN_PATTERNS = (
    (re.compile(r"\bnew\b"), "new"),
    (re.compile(r"\b(malloc|calloc|realloc|aligned_alloc|alloca)\s*\("), None),
    (re.compile(r"\bstd\s*::\s*(vector|deque|list|map|multimap|set|multiset|"
                r"unordered_map|unordered_set|basic_string|string)\b"), "standard container"),
    (re.compile(r"\b(?:bool|char|short|int|long|float|double|u?int(?:8|16|32|64)_t)"
                r"\s+[A-Za-z_]\w*\s*\["), "auxiliary array"),
)


def strip_comments_and_literals(source):
    result = []
    index = 0
    state = "code"
    quote = ""
    while index < len(source):
        current = source[index]
        following = source[index + 1] if index + 1 < len(source) else ""
        if state == "code":
            if current == "/" and following == "/":
                result.extend("  ")
                index += 2
                state = "line_comment"
                continue
            if current == "/" and following == "*":
                result.extend("  ")
                index += 2
                state = "block_comment"
                continue
            if current in ('"', "'"):
                quote = current
                result.append(" ")
                index += 1
                state = "literal"
                continue
            result.append(current)
            index += 1
            continue
        if state == "line_comment":
            result.append("\n" if current == "\n" else " ")
            index += 1
            if current == "\n":
                state = "code"
            continue
        if state == "block_comment":
            if current == "*" and following == "/":
                result.extend("  ")
                index += 2
                state = "code"
            else:
                result.append("\n" if current == "\n" else " ")
                index += 1
            continue
        if current == "\\":
            result.append(" ")
            if following:
                result.append("\n" if following == "\n" else " ")
                index += 2
            else:
                index += 1
        elif current == quote:
            result.append(" ")
            index += 1
            state = "code"
        else:
            result.append("\n" if current == "\n" else " ")
            index += 1
    return "".join(result)


def find_forbidden(source):
    source = strip_comments_and_literals(source)
    for pattern, label in FORBIDDEN_PATTERNS:
        match = pattern.search(source)
        if match:
            return label or match.group(1)
    return None


def main():
    if len(sys.argv) != 3:
        print("usage: check_no_allocation.py TEST_PROGRAM SOURCE", file=sys.stderr)
        return 2
    test_program, source_path = sys.argv[1:]
    forbidden = find_forbidden(Path(source_path).read_text(encoding="utf-8"))
    if forbidden:
        print("update_step source uses forbidden allocation function: " + forbidden,
              file=sys.stderr)
        return 1
    return subprocess.run([test_program, "allocation"], stdin=subprocess.DEVNULL).returncode


if __name__ == "__main__":
    sys.exit(main())
