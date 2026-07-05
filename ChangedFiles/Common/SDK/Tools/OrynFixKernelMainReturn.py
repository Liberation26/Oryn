#!/usr/bin/env python3
import argparse
import pathlib
import re
import sys

OLD_EXACT = """#include \"OrynKernelSdk.h\"

static void Kernel5Main(OrynKernelSdkContext* kernel)
{
    OrynKernelSdkWriteLine(kernel, \"Hello World\");
}

ORYN_KERNEL_APPLICATION(\"Kernel-5\", Kernel5Main)
"""

NEW_EXACT = """#include \"OrynKernelSdk.h\"

static OrynStatus Kernel5Main(OrynKernelSdkContext* kernel)
{
    OrynStatus status;

    if (kernel == 0)
    {
        return OrynStatusInvalidArgument(\"Kernel SDK context was null.\");
    }

    status = OrynKernelSdkWriteLine(kernel, \"Hello World\");
    if (ORYN_STATUS_FAILED(status))
    {
        return status;
    }

    return OrynStatusOk(\"Kernel-5 completed.\");
}

ORYN_KERNEL_APPLICATION(\"Kernel-5\", Kernel5Main)
"""

ENTRY_RE = re.compile(
    r'static\s+void\s+(Kernel\w*Main)\s*\(\s*OrynKernelSdkContext\s*\*\s*(\w+)\s*\)\s*\{(?P<body>.*?)\}\s*\n\s*ORYN_KERNEL_APPLICATION\(',
    re.S)

WRITE_RE = re.compile(r'OrynKernelSdkWriteLine\s*\(\s*(\w+)\s*,\s*(\"(?:[^\"\\]|\\.)*\")\s*\)\s*;')


def read_text(path):
    data = path.read_bytes()
    return data.decode('utf-8-sig')


def write_text(path, text):
    path.write_text(text.replace('\n', '\r\n'), encoding='utf-8')


def convert_file(path):
    text = read_text(path)
    normalized = text.replace('\r\n', '\n')

    if OLD_EXACT in normalized:
        write_text(path, normalized.replace(OLD_EXACT, NEW_EXACT))
        return True, 'exact Kernel-5 hello-world entry converted'

    match = ENTRY_RE.search(normalized)
    if not match:
        return False, 'no void Oryn kernel app entry found'

    name = match.group(1)
    ctx = match.group(2)
    body = match.group('body')
    write_match = WRITE_RE.search(body)

    if not write_match:
        return False, 'void entry found, but no simple OrynKernelSdkWriteLine call was found'

    message = write_match.group(2)
    replacement = f'''static OrynStatus {name}(OrynKernelSdkContext* {ctx})
{{
    OrynStatus status;

    if ({ctx} == 0)
    {{
        return OrynStatusInvalidArgument("Kernel SDK context was null.");
    }}

    status = OrynKernelSdkWriteLine({ctx}, {message});
    if (ORYN_STATUS_FAILED(status))
    {{
        return status;
    }}

    return OrynStatusOk("{name} completed.");
}}

ORYN_KERNEL_APPLICATION('''

    converted = normalized[:match.start()] + replacement + normalized[match.end():]
    write_text(path, converted)
    return True, f'{name} converted to OrynStatus'


def main():
    parser = argparse.ArgumentParser(description='Convert Oryn KernelMain void app entry points to OrynStatus entries.')
    parser.add_argument('root', nargs='?', default='.', help='SDK/project root')
    args = parser.parse_args()

    root = pathlib.Path(args.root).resolve()
    candidates = list(root.rglob('KernelMain.c')) + list(root.rglob('*KernelMain*.c'))
    seen = set()
    changed = 0

    for path in candidates:
        if path in seen:
            continue
        seen.add(path)
        try:
            ok, message = convert_file(path)
        except UnicodeDecodeError:
            continue
        if ok:
            changed += 1
            print(f'[ OK ] {path}: {message}')

    if changed == 0:
        print('[WARN] No KernelMain file was converted.')
        return 1

    print(f'[ OK ] Converted {changed} KernelMain file(s).')
    return 0

if __name__ == '__main__':
    sys.exit(main())
