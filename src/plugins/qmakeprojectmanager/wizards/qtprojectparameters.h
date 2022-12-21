// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QStringList>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace QmakeProjectManager {
namespace Internal {

// Create a macro name by taking a file name, upper casing it and
// appending a suffix.
QString createMacro(const QString &name, const QString &suffix);

// Base parameters for application project generation with functionality to
// write a .pro-file section.

struct QtProjectParameters
{
    enum Type { ConsoleApp, GuiApp, StaticLibrary, SharedLibrary, QtPlugin, EmptyProject };
    enum QtVersionSupport { SupportQt4And5, SupportQt4Only, SupportQt5Only };
    enum Flags { WidgetsRequiredFlag = 0x1 };

    QtProjectParameters();
    // Return project path as "path/name"
    Utils::FilePath projectPath() const;
    void writeProFile(QTextStream &) const;
    static void writeProFileHeader(QTextStream &);

    // Shared library: Name of export macro (XXX_EXPORT)
    static QString exportMacro(const QString &projectName);
    // Shared library: name of #define indicating compilation within library
    static QString libraryMacro(const QString &projectName);

    Type type = ConsoleApp;
    unsigned flags = 0;
    QtVersionSupport qtVersionSupport = SupportQt4And5;
    QString fileName;
    QString target;
    Utils::FilePath path;
    QStringList selectedModules;
    QStringList deselectedModules;
    QString targetDirectory;
};

} // namespace Internal
} // namespace QmakeProjectManager
