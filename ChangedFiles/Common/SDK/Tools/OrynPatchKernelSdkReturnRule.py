#!/usr/bin/env python3
import argparse
import pathlib
import re
import sys


def read_text(path):
    return path.read_bytes().decode('utf-8-sig')


def write_text(path, text):
    path.write_text(text.replace('\n', '\r\n'), encoding='utf-8')


def patch_header(path):
    text = read_text(path).replace('\r\n', '\n')
    original = text

    if '#include "OrynStatus.h"' not in text:
        lines = text.split('\n')
        insert_at = 0
        while insert_at < len(lines) and lines[insert_at].startswith('#'):
            insert_at += 1
        lines.insert(insert_at, '#include "OrynStatus.h"')
        text = '\n'.join(lines)

    text = re.sub(
        r'typedef\s+void\s*\(\s*\*\s*OrynKernelApplication(?:Main|Entry)\s*\)\s*\(\s*OrynKernelSdkContext\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)?\s*\)\s*;',
        'typedef OrynStatus (*OrynKernelApplicationEntry)(OrynKernelSdkContext* kernel);',
        text)

    text = re.sub(r'\bvoid\s+OrynKernelSdkWriteLine\s*\(', 'OrynStatus OrynKernelSdkWriteLine(', text)
    text = re.sub(r'\bvoid\s+OrynKernelSdkWrite\s*\(', 'OrynStatus OrynKernelSdkWrite(', text)

    if text != original:
        write_text(path, text)
        return True
    return False


def patch_source(path):
    text = read_text(path).replace('\r\n', '\n')
    original = text

    if '#include "OrynStatus.h"' not in text and 'OrynKernelSdkWrite' in text:
        text = text.replace('#include "OrynKernelSdk.h"', '#include "OrynKernelSdk.h"\n#include "OrynStatus.h"')

    text = re.sub(r'\bvoid\s+OrynKernelSdkWriteLine\s*\(', 'OrynStatus OrynKernelSdkWriteLine(', text)
    text = re.sub(r'\bvoid\s+OrynKernelSdkWrite\s*\(', 'OrynStatus OrynKernelSdkWrite(', text)

    text = re.sub(
        r'(OrynStatus\s+OrynKernelSdkWriteLine\s*\([^)]*\)\s*\{(?:(?!\n\}).)*?)(\n\})',
        lambda m: m.group(1) + ('\n    return OrynStatusOk("Kernel SDK write line completed.");' if 'return OrynStatus' not in m.group(1) else '') + m.group(2),
        text,
        flags=re.S)

    text = re.sub(
        r'(OrynStatus\s+OrynKernelSdkWrite\s*\([^)]*\)\s*\{(?:(?!\n\}).)*?)(\n\})',
        lambda m: m.group(1) + ('\n    return OrynStatusOk("Kernel SDK write completed.");' if 'return OrynStatus' not in m.group(1) else '') + m.group(2),
        text,
        flags=re.S)

    if text != original:
        write_text(path, text)
        return True
    return False


def main():
    parser = argparse.ArgumentParser(description='Patch OrynKernelSdk declarations toward OrynStatus returns.')
    parser.add_argument('root', nargs='?', default='.', help='SDK/project root')
    args = parser.parse_args()

    root = pathlib.Path(args.root).resolve()
    changed = 0

    for path in root.rglob('OrynKernelSdk.h'):
        if patch_header(path):
            changed += 1
            print(f'[ OK ] Patched header: {path}')

    for path in root.rglob('*.c'):
        try:
            content = read_text(path)
        except UnicodeDecodeError:
            continue
        if 'OrynKernelSdkWrite' in content:
            if patch_source(path):
                changed += 1
                print(f'[ OK ] Patched source: {path}')

    if changed == 0:
        print('[WARN] No OrynKernelSdk declaration/source files were patched.')
        return 1

    print(f'[ OK ] Patched {changed} SDK file(s).')
    return 0

if __name__ == '__main__':
    sys.exit(main())
