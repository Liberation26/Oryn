#!/usr/bin/env python3
from __future__ import annotations
import sys
from pathlib import Path

FUNCTIONS = {
    "printf": "OrynSDK/Common/OrynLibC/Source/stdio/printf.c",
    "vprintf": "OrynSDK/Common/OrynLibC/Source/stdio/vprintf.c",
    "snprintf": "OrynSDK/Common/OrynLibC/Source/stdio/snprintf.c",
    "vsnprintf": "OrynSDK/Common/OrynLibC/Source/stdio/vsnprintf.c",
}

CANDIDATE_NAMES = [
    "LibCFunctionUnitManifest.txt",
    "OrynLibCFunctionUnitManifest.txt",
    "FunctionUnitManifest.txt",
    "LibCManifest.txt",
    "manifest.txt",
]

SEARCH_DIR_HINTS = [
    "OrynSDK/Common/OrynLibC",
    "OrynSDK/Common/OrynLibC/Manifest",
    "OrynSDK/Common/OrynLibC/Manifests",
    "OrynSDK/Common/SDK/Manifests",
    "OrynProjects/Kernel-5/Build/Plan",
]


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return path.read_text(encoding="utf-8", errors="replace")


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")


def likely_manifest(path: Path) -> bool:
    name = path.name.lower()
    if "manifest" not in name:
        return False
    if "libc" in str(path).lower() or "function" in name or "unit" in name:
        return True
    return False


def find_manifests(root: Path) -> list[Path]:
    found: list[Path] = []
    for hint in SEARCH_DIR_HINTS:
        d = root / hint
        if d.exists():
            for p in d.rglob("*"):
                if p.is_file() and likely_manifest(p):
                    found.append(p)
    for name in CANDIDATE_NAMES:
        for p in root.rglob(name):
            if p.is_file() and likely_manifest(p):
                found.append(p)
    # Stable unique order
    result = []
    seen = set()
    for p in found:
        rp = p.resolve()
        if rp not in seen:
            seen.add(rp)
            result.append(p)
    return result


def line_for(style: str, name: str, rel_source: str) -> str:
    if style == "pipe4":
        return f"{name}|stdio|{rel_source}|kernel,userland"
    if style == "csv":
        return f"{name},stdio,{rel_source},kernel,userland"
    if style == "jsonish":
        return f'{{"name":"{name}","module":"stdio","source":"{rel_source}","targets":"kernel,userland"}}'
    return f"{name} {rel_source}"


def detect_style(text: str) -> str:
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "|" in line:
            return "pipe4"
        if line.startswith("{") and "source" in line:
            return "jsonish"
        if "," in line:
            return "csv"
    return "plain"


def patch_manifest(path: Path, root: Path) -> bool:
    text = read_text(path)
    before = text
    style = detect_style(text)
    lower = text.lower()
    additions = []
    for name, rel_source in FUNCTIONS.items():
        source_path = root / rel_source
        if not source_path.exists():
            print(f"[WARN] Source file missing for {name}: {source_path}")
        has_name = name.lower() in lower
        has_source = rel_source.lower() in lower or ("source/stdio/" + name + ".c") in lower
        if not (has_name and has_source):
            additions.append(line_for(style, name, rel_source))
    if additions:
        if text and not text.endswith("\n"):
            text += "\n"
        text += "\n# Added by OrynWsl 0.5.166: printf function units\n"
        text += "\n".join(additions)
        text += "\n"
    if text != before:
        backup = path.with_suffix(path.suffix + ".bak-0.5.166")
        if not backup.exists():
            write_text(backup, before)
        write_text(path, text)
        return True
    return False


def ensure_sources(root: Path) -> None:
    include = root / "OrynSDK/Common/OrynLibC/Include/stdio.h"
    if include.exists():
        text = read_text(include)
        changed = False
        prototypes = [
            "int printf(const char* restrict format, ...);",
            "int vprintf(const char* restrict format, va_list args);",
            "int snprintf(char* restrict target, size_t size, const char* restrict format, ...);",
            "int vsnprintf(char* restrict target, size_t size, const char* restrict format, va_list args);",
        ]
        if "va_list" in text and "#include <stdarg.h>" not in text:
            text = "#include <stdarg.h>\n" + text
            changed = True
        for proto in prototypes:
            if proto not in text:
                if text and not text.endswith("\n"):
                    text += "\n"
                text += proto + "\n"
                changed = True
        if changed:
            write_text(include, text)
            print(f"[ OK ] Patched stdio prototypes: {include}")


def main(argv: list[str]) -> int:
    root = Path(argv[1]).resolve() if len(argv) > 1 else Path.cwd().resolve()
    ensure_sources(root)
    manifests = find_manifests(root)
    if not manifests:
        print("[FAIL] No LibC/function manifest files found.")
        print("[INFO] Please show the report at OrynProjects/Kernel-5/Build/Plan/LibCFunctionUnitManifestReport.txt")
        return 2
    patched = 0
    for manifest in manifests:
        try:
            if patch_manifest(manifest, root):
                patched += 1
                print(f"[ OK ] Patched LibC manifest: {manifest}")
            else:
                print(f"[ OK ] LibC manifest already contains printf units: {manifest}")
        except Exception as exc:
            print(f"[WARN] Could not patch {manifest}: {exc}")
    print(f"[ OK ] Checked {len(manifests)} manifest file(s), patched {patched}.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
