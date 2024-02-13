// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Utils { class FilePath; }

namespace Haskell::Internal {

Utils::FilePath findProjectDirectory(const Utils::FilePath &filePath);
void openGhci(const Utils::FilePath &haskellFile);

void setupHaskellActions(QObject *guard);

} // Haskell::Internal
