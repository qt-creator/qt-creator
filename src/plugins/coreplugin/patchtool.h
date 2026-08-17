// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/filepath.h>

#include <functional>

namespace Core {

enum class PatchAction {
    Apply,
    Revert
};

class CORE_EXPORT PatchTool
{
public:
    static Utils::FilePath patchCommand();

    static bool confirmPatching(QWidget *parent, PatchAction patchAction, bool isModified);

    // Utility to run the 'patch' command. When a logger is given, the progress
    // and error messages that otherwise go to the message pane are handed to it
    // instead, so a headless caller can capture the patch tool's output.
    static bool runPatch(const QByteArray &input, const Utils::FilePath &workingDirectory = {},
                         int strip = 0, PatchAction patchAction = PatchAction::Apply,
                         const std::function<void(const QString &)> &logger = {});
};

} // namespace Core
