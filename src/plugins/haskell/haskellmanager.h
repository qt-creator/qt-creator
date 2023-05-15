// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include <utils/fileutils.h>

namespace Haskell::Internal {

class HaskellManager
{
public:
    static HaskellManager *instance();

    static Utils::FilePath findProjectDirectory(const Utils::FilePath &filePath);
    static void openGhci(const Utils::FilePath &haskellFile);
};

} // Haskell::Internal
