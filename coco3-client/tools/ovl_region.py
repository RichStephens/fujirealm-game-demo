#!/usr/bin/env python3
"""Print the overlay region's start or end address (hex, for cmoc --org and
ovl_pack.py), read from the game's linker map."""

from __future__ import annotations

import re
import sys

SYMBOLS = {"start": "_ovl_region_start", "end": "_ovl_region_end"}


def main() -> int:
    if len(sys.argv) != 3 or sys.argv[1] not in SYMBOLS:
        print("usage: ovl_region.py start|end GAME.map", file=sys.stderr)
        return 2
    pattern = re.compile(rf"Symbol: {SYMBOLS[sys.argv[1]]} \(\S+\) = ([0-9A-F]+)")
    with open(sys.argv[2], encoding="ascii") as f:
        for line in f:
            match = pattern.search(line)
            if match:
                print(match.group(1))
                return 0
    print(f"{SYMBOLS[sys.argv[1]]} not in {sys.argv[2]}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
