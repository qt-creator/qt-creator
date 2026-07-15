// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Fallback file watching backend for platforms without one. Implements the
// interface declared in watcher.c. Included by cmdbridge.c — do not compile
// separately.

#if !defined(__linux__) && !defined(_WIN32) && !defined(__APPLE__) && !defined(__FreeBSD__) \
    && !defined(__OpenBSD__) && !defined(__NetBSD__)

static bool watch_backend_supported(void)
{
    return false;
}

static void watch_backend_start(void) {}

static int watch_backend_find(const char *path)
{
    (void) path;
    return -1;
}

static int watch_backend_add(const char *path)
{
    (void) path;
    return -1;
}

static void watch_backend_remove(int idx)
{
    (void) idx;
}

#endif
