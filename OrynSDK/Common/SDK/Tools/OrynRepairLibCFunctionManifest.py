#!/usr/bin/env python3
from pathlib import Path
import sys

UNITS = {
    "printf": "OrynSDK/Common/OrynLibC/Source/stdio/printf.c",
    "vprintf": "OrynSDK/Common/OrynLibC/Source/stdio/vprintf.c",
    "snprintf": "OrynSDK/Common/OrynLibC/Source/stdio/snprintf.c",
    "vsnprintf": "OrynSDK/Common/OrynLibC/Source/stdio/vsnprintf.c",
}


def main() -> int:
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()
    libc = root / "OrynSDK" / "Common" / "OrynLibC"
    if not libc.exists():
        print(f"[FAIL] Cannot find {libc}")
        return 1
    manifest_dir = libc / "Manifest" / "stdio"
    manifest_dir.mkdir(parents=True, exist_ok=True)
    for name, source in UNITS.items():
        path = manifest_dir / f"{name}.manifest"
        path.write_text(
            f"Name: {name}\nModule: OrynLibC\nCategory: stdio\nHeader: OrynSDK/Common/OrynLibC/Include/stdio.h\nSource: {source}\nPublic: yes\nReturn: int\nStatus: OK\n",
            encoding="utf-8",
            newline="\n",
        )
        beside = libc / "Source" / "stdio" / f"{name}.c.manifest"
        beside.write_text(
            f"Name: {name}\nModule: OrynLibC\nCategory: stdio\nHeader: ../../Include/stdio.h\nSource: {name}.c\nPublic: yes\nReturn: int\nStatus: OK\n",
            encoding="utf-8",
            newline="\n",
        )
        print(f"[ OK ] Manifest written: {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
