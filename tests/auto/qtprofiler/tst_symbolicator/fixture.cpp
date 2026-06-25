// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// A tiny dylib the symbolicator test dlopen()s at runtime to stand in for a
// library loaded into the target while a recording is in progress (e.g. a
// dlopen()'d plugin). It deliberately does not get linked into the test binary,
// so the symbol below only becomes resolvable after the test loads it.
//
// The function is exported with default visibility and a C linkage name so the
// test can both dlsym() its address and assert on the exact symbol name the
// symbolicator reports for it. noinline keeps it as a distinct symbol.

extern "C" __attribute__((visibility("default"), noinline))
int qtprofiler_symbolicatorFixtureFunction(int seed)
{
    volatile int acc = seed;
    for (int i = 0; i < 16; ++i)
        acc += i * seed + 1;
    return acc;
}
