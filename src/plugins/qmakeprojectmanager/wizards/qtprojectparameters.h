// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QStringList>

namespace QmakeProjectManager::Internal {

// Base parameters for application project generation with functionality to
// write a .pro-file section.
struct QtProjectParameters
{
    enum Type { ConsoleApp, GuiApp, StaticLibrary, SharedLibrary, QtPlugin, EmptyProject };
    enum QtVersionSupport { SupportQt4And5, SupportQt4Only, SupportQt5Only };

    // Return project path as "path/name"
    Utils::FilePath projectPath() const { return path / fileName; }

    Type type = ConsoleApp;
    QtVersionSupport qtVersionSupport = SupportQt4And5;
    QString fileName;
    QString target;
    Utils::FilePath path;
    QStringList selectedModules;
    QStringList deselectedModules;
    QString targetDirectory;
};

} // namespace QmakeProjectManager::Internal
