// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/environment.h>
#include <utils/filepath.h>

#include <optional>

namespace Debugger::Internal {

struct RrLaunchLine
{
    QString channel;
    Utils::FilePath executable;
};

// Reads the server address and the executable from rr's "Launch gdb with" output.
std::optional<RrLaunchLine> parseRrLaunchLine(const QString &output);

// Where rr record puts its traces when it runs in the given environment.
Utils::FilePath rrTraceRoot(const Utils::Environment &env);

void setupRrSupport();

} // Debugger::Internal
