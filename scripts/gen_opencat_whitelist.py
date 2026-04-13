#!/usr/bin/env python3
"""Generate OpenCat whitelist initializer from InstinctBittleESP.h."""

import argparse
import pathlib
import re
import sys


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Extract OpenCat skills and print C++ initializer entries."
    )
    parser.add_argument(
        "input",
        type=pathlib.Path,
        help="Path to InstinctBittleESP.h",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    text = args.input.read_text(encoding="utf-8", errors="ignore")

    match = re.search(r"skillNameWithType\[\]\s*=\s*\{([^}]*)\}", text, re.DOTALL)
    if not match:
        print("error: cannot find skillNameWithType[]", file=sys.stderr)
        return 1

    raw_items = re.findall(r'"([^"]+)"', match.group(1))
    skills = [item[:-1] for item in raw_items if len(item) >= 1]

    for skill in skills:
        print(f'"{skill}",')

    print(f"// total: {len(skills)}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
