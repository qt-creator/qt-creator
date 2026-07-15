#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

"""
Fuzzer for CmdBridge CBOR parser.
Feeds malformed CBOR data to the cmdbridge binary and detects crashes.

Usage:
    python3 cbor_fuzzer.py                    # Auto-detect binary
    python3 cbor_fuzzer.py /path/to/binary    # Specify binary
    FUZZ_ITERATIONS=100000 python3 cbor_fuzzer.py

The fuzzer searches for the C implementation of CmdBridge in common
build locations. Set CMDBRIDGE_BIN environment variable to override.
"""

import subprocess
import sys
import os
import struct
import random
import hashlib
import time


def find_cmdbridge_binary():
    """Find the cmdbridge C binary in common build locations."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.abspath(os.path.join(script_dir, "..", "..", "..", ".."))

    # Check for zig-built binaries (both arm64 and amd64)
    build_dirs = [
        os.path.join(repo_root, "build", "macOS-debug"),
        os.path.join(repo_root, "build", "macOS-release"),
        os.path.join(repo_root, "build", "debug"),
        os.path.join(repo_root, "build", "release"),
    ]

    # Also check relative to repo root in case of different layout
    build_dirs += [
        os.path.join(repo_root, "build"),
    ]

    binary_names = [
        "cmdbridge-darwin-arm64",
        "cmdbridge-darwin-amd64",
        "cmdbridge-linux-arm64",
        "cmdbridge-linux-amd64",
        "cmdbridge-windows-arm64.exe",
        "cmdbridge-windows-amd64.exe",
    ]

    # Add platform-specific names
    if sys.platform == "darwin":
        binary_names = [
            "cmdbridge-darwin-arm64",
            "cmdbridge-darwin-amd64",
        ]
    elif sys.platform == "linux":
        binary_names = [
            "cmdbridge-linux-arm64",
            "cmdbridge-linux-amd64",
        ]
    elif sys.platform == "win32":
        binary_names = [
            "cmdbridge-windows-arm64.exe",
            "cmdbridge-windows-amd64.exe",
        ]

    for build_dir in build_dirs:
        if not os.path.isdir(build_dir):
            continue
        for name in binary_names:
            path = os.path.join(build_dir, name)
            if os.path.isfile(path) and os.access(path, os.X_OK):
                return path
            # Check inside Qt Creator.app on macOS
            app_path = os.path.join(
                build_dir, "Qt Creator.app", "Contents", "Resources", "libexec", name
            )
            if os.path.isfile(app_path):
                return app_path

    return None


def enc_uint(v):
    """Encode a CBOR unsigned integer."""
    if v < 24:
        return bytes([v])
    elif v <= 0xFF:
        return bytes([24, v])
    elif v <= 0xFFFF:
        return bytes([25]) + struct.pack(">H", v)
    elif v <= 0xFFFFFFFF:
        return bytes([26]) + struct.pack(">I", v)
    else:
        return bytes([27]) + struct.pack(">Q", v)


def enc_text(s):
    """Encode a CBOR text string."""
    b = s.encode("utf-8")
    h = enc_uint(len(b))
    return bytes([0x60 | h[0]]) + h[1:] + b


def enc_bytes_val(b):
    """Encode a CBOR byte string."""
    h = enc_uint(len(b))
    return bytes([0x40 | h[0]]) + h[1:] + b


def enc_array(items):
    """Encode a CBOR array (items is concatenated child bytes)."""
    h = enc_uint(len(items))
    return bytes([0x80 | h[0]]) + h[1:] + items


def enc_map(pairs):
    """Encode a CBOR map (pairs is concatenated key-value bytes)."""
    h = enc_uint(len(pairs) // 2)
    return bytes([0xA0 | h[0]]) + h[1:] + pairs


def rand_bytes(n):
    """Generate n random bytes."""
    return bytes([random.randint(0, 255) for _ in range(n)])


def main():
    # Determine binary path
    if len(sys.argv) > 1:
        cmdbridge_bin = sys.argv[1]
    else:
        cmdbridge_bin = os.environ.get("CMDBRIDGE_BIN", "")
        if not cmdbridge_bin:
            cmdbridge_bin = find_cmdbridge_binary()

    if not cmdbridge_bin or not os.path.isfile(cmdbridge_bin):
        print("Error: cmdbridge binary not found.")
        print("Usage: python3 cbor_fuzzer.py [path/to/cmdbridge-binary]")
        print("   or: CMDBRIDGE_BIN=/path/to/binary python3 cbor_fuzzer.py")
        sys.exit(1)

    num_iterations = int(os.environ.get("FUZZ_ITERATIONS", "50000"))
    total_tests = 0
    crashes = 0
    crash_inputs = []
    proc = None

    def spawn():
        nonlocal proc
        proc = subprocess.Popen(
            [cmdbridge_bin],
            stdin=subprocess.PIPE,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
        )

    def feed(data, desc):
        nonlocal total_tests, crashes
        total_tests += 1

        try:
            proc.stdin.write(data)
            proc.stdin.flush()
            time.sleep(0.001)
        except (BrokenPipeError, OSError):
            crashes += 1
            crash_inputs.append((desc, data, proc.stderr.read()))
            print(f"\n*** CRASH #{crashes}: {desc}")
            print(f"    Input ({len(data)} bytes): {data[:100].hex()}")
            try:
                proc.wait(timeout=1)
                print(f"    Exit code: {proc.returncode}")
            except Exception:
                proc.kill()
                proc.wait()
            return True
        return False

    def restart():
        nonlocal proc
        try:
            proc.terminate()
            proc.wait(timeout=2)
        except Exception:
            proc.kill()
            proc.wait()
        spawn()
        time.sleep(0.1)

    print(f"Fuzzing: {cmdbridge_bin}")
    print(f"Iterations: {num_iterations}")
    print()

    spawn()
    time.sleep(0.2)

    # Phase 1: All single bytes
    print("Phase 1: Single bytes (0x00-0xFF)...")
    for b in range(256):
        if feed(bytes([b]), f"byte(0x{b:02x})"):
            restart()

    # Phase 2: Valid CBOR + garbage
    print("Phase 2: Valid CBOR + garbage...")
    valid = enc_map(enc_text("Type") + enc_text("ping"))
    for glen in [1, 3, 7, 15, 31, 63, 127]:
        for gb in [0x00, 0xFF, 0x80, 0xC0, 0xF8, 0xFE]:
            if feed(valid + bytes([gb]) * glen, f"valid+garbage({glen},0x{gb:02x})"):
                restart()

    # Phase 3: Truncated CBOR
    print("Phase 3: Truncated CBOR...")
    big = enc_map(
        enc_text("Type")
        + enc_text("writefile")
        + enc_text("Id")
        + enc_uint(1)
        + enc_text("Path")
        + enc_text("/tmp/test")
        + enc_text("Contents")
        + enc_bytes_val(b"X" * 200)
    )
    for t in range(1, len(big)):
        if feed(big[:t], f"trunc({t}/{len(big)})"):
            restart()

    # Phase 4: Huge length fields (overflow attempts)
    print("Phase 4: Overflow attempts...")
    names = {0x60: "text", 0x40: "bytes", 0x80: "array", 0xA0: "map"}
    for maj in [0x60, 0x40, 0x80, 0xA0]:
        for eb in [1, 2, 4, 8]:
            if eb == 1:
                hdr = bytes([maj | 24, 0xFF])
            elif eb == 2:
                hdr = bytes([maj | 25, 0xFF, 0xFF])
            elif eb == 4:
                hdr = bytes([maj | 26, 0xFF, 0xFF, 0xFF, 0xFF])
            else:
                hdr = bytes([maj | 27]) + b"\xFF" * 8
            for plen in [0, 1, 2, 4, 8, 16]:
                if feed(hdr + rand_bytes(plen), f"{names[maj]}_huge({eb}b,pay{plen})"):
                    restart()

    # Phase 5: Deep nesting
    print("Phase 5: Deep nesting...")
    for depth in [100, 200, 400]:
        if feed(b"\x81" * depth + b"\x00" * depth, f"deep_array({depth})"):
            restart()

    for depth in [50, 100, 200]:
        if feed(
            b"\xA2" * depth + enc_text("k") + enc_text("v") * (depth * 2),
            f"deep_map({depth})",
        ):
            restart()

    # Phase 6: Random fuzzing
    print("Phase 6: Random fuzzing...")
    random.seed(42)
    for i in range(num_iterations):
        size = random.randint(1, 256)
        if random.random() < 0.3:
            maj = random.choice([0x00, 0x40, 0x60, 0x80, 0xA0])
            data = bytes([maj | random.randint(0, 7)])
            data += rand_bytes(random.randint(0, min(size, 128)))
        else:
            data = rand_bytes(size)
        if feed(data, f"random({i},size={size})"):
            restart()
        if (i + 1) % 10000 == 0:
            print(f"  {i+1}/{num_iterations} tests so far...", flush=True)

    # Phase 7: Invalid UTF-8
    print("Phase 7: Invalid UTF-8...")
    for seq in [b"\xFF\xFE", b"\xC0\x80", b"\xF4\x90\x80\x80", b"\xED\xA0\x80"]:
        if feed(enc_text(seq.decode("latin-1")), f"bad_utf8({seq.hex()})"):
            restart()

    # Phase 8: Streaming
    print("Phase 8: Streaming...")
    ping = enc_map(enc_text("Type") + enc_text("ping"))
    for rep in [10, 50, 100, 500]:
        if feed(ping * rep, f"stream({rep})"):
            restart()

    # Clean shutdown
    try:
        proc.stdin.write(enc_map(enc_text("Type") + enc_text("exit")))
        proc.stdin.flush()
        time.sleep(0.3)
        proc.terminate()
        proc.wait(timeout=2)
    except Exception:
        proc.kill()
        proc.wait()

    print(f"\n{'=' * 60}")
    print(f"RESULTS: {total_tests} tests, {crashes} crashes")
    if crash_inputs:
        print(f"\nCrash details:")
        for desc, data, stderr in crash_inputs:
            print(f"\n  Crash: {desc}")
            print(f"  Input ({len(data)} bytes): {data[:80].hex()}")
            print(f"  Stderr: {stderr[:200]}")
    print(f"{'=' * 60}")

    return crashes > 0


if __name__ == "__main__":
    crash_found = main()
    sys.exit(1 if crash_found else 0)
