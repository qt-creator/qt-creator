// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

namespace VcsBase {

enum class RunFlags {
    None                   = 0,        // Empty
    ShowStdOut             = (1 << 0), // Show standard output.
    MergeOutputChannels    = (1 << 1), // See QProcess: Merge stderr/stdout.
    SuppressStdErr         = (1 << 2), // Suppress standard error output.
    SuppressFailMessage    = (1 << 3), // No message about command failure.
    SuppressCommandLogging = (1 << 4), // No command log entry.
    ShowSuccessMessage     = (1 << 5), // Show message about successful completion of command.
    ForceCLocale           = (1 << 6), // Force C-locale for commands whose output is parsed.
    SilentOutput           = (1 << 7), // Suppress user notifications about the output happening.
    UseEventLoop           = (1 << 8), // Use event loop when executed in UI thread.
    ExpectRepoChanges      = (1 << 9), // Expect changes in repository by the command
    NoOutput = SuppressStdErr | SuppressFailMessage | SuppressCommandLogging
};

inline void VCSBASE_EXPORT operator|=(RunFlags &p, RunFlags r)
{
    p = RunFlags(int(p) | int(r));
}

inline RunFlags VCSBASE_EXPORT operator|(RunFlags p, RunFlags r)
{
    return RunFlags(int(p) | int(r));
}

inline void VCSBASE_EXPORT operator&=(RunFlags &p, RunFlags r)
{
    p = RunFlags(int(p) & int(r));
}

// Note, that it returns bool, not RunFlags.
// It's only meant for testing whether a specific bit is set.
inline bool VCSBASE_EXPORT operator&(RunFlags p, RunFlags r)
{
    return bool(int(p) & int(r));
}

} // namespace VcsBase
