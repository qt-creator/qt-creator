// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <fcntl.h>

#ifdef Q_OS_WIN
#include <libeu_compat.h>
#else
#include <cstdlib>
#include <unistd.h>
#endif

namespace Profiler::Internal {

// On Windows, elfutils comes with a C runtime of its own. The descriptors it
// reads from and the memory it returns are that runtime's, so they have to go
// through it rather than through this one.

// A descriptor for elfutils to read `path` through, or -1.
inline int openForElfutils(const Utils::FilePath &path)
{
    const QByteArray fileName = path.toFSPathString().toLocal8Bit();
#ifdef Q_OS_WIN
    // The counterpart of O_CLOEXEC: keep child processes from inheriting it.
    return eu_compat_open(fileName.constData(), O_RDONLY | O_BINARY | _O_NOINHERIT);
#else
    return ::open(fileName.constData(), O_RDONLY | O_CLOEXEC);
#endif
}

inline void closeForElfutils(int fd)
{
#ifdef Q_OS_WIN
    eu_compat_close(fd);
#else
    ::close(fd);
#endif
}

// Frees what elfutils allocated and leaves to the caller.
inline void freeFromElfutils(void *memory)
{
#ifdef Q_OS_WIN
    eu_compat_free(memory);
#else
    std::free(memory);
#endif
}

} // namespace Profiler::Internal
