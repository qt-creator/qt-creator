// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <QMetaObject>

namespace VcsBase {

Q_NAMESPACE_EXPORT(VCSBASE_EXPORT)

enum class RunFlag {
    None                   = 0,         // Empty.
    // Process related
    MergeOutputChannels    = (1 << 0), // See QProcess::ProcessChannelMode::MergedChannels.
    ForceCLocale           = (1 << 1), // Force C-locale, sets LANG and LANGUAGE env vars to "C".
    // Decorator related
    SuppressStdErr         = (1 << 2), // Suppress standard error output.
    SuppressFailMessage    = (1 << 3), // No message about command failure.
    SuppressCommandLogging = (1 << 4), // No starting command log entry.
    ShowSuccessMessage     = (1 << 5), // Show message about successful completion of command.
    ShowStdOut             = (1 << 6), // Show standard output.
    ExpectRepoChanges      = (1 << 7), // Expect changes in repository by the command.
    NoOutput               = SuppressStdErr | SuppressFailMessage | SuppressCommandLogging
};
Q_DECLARE_FLAGS(RunFlags, RunFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(RunFlags)
Q_FLAG_NS(RunFlags)

} // namespace VcsBase
