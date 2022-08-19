// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/filepath.h>

namespace Core {

class CORE_EXPORT PatchTool
{
public:
    static Utils::FilePath patchCommand();
    static void setPatchCommand(const Utils::FilePath &newCommand);

    // Utility to run the 'patch' command
    static bool runPatch(const QByteArray &input, const Utils::FilePath &workingDirectory = {},
                         int strip = 0, bool reverse = false);
};

} // namespace Core
