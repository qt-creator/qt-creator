// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

/* Helper functions to get the last core from systemd/coredumpctl */
namespace Debugger::Internal {

struct LastCore
{
    operator bool() const { return !binary.isEmpty() && !coreFile.isEmpty(); }

    Utils::FilePath binary;
    Utils::FilePath coreFile;
};

LastCore getLastCore();

} // namespace Debugger::Internal
