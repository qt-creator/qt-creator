// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/cppprojectfile.h>

#include <QHash>

namespace ProjectExplorer {
class HeaderPath;
class Macro;
}

namespace CompilationDatabaseProjectManager {
namespace Internal {

class DbEntry {
public:
    QStringList flags;
    Utils::FilePath fileName;
    Utils::FilePath workingDir;
};

class DbContents {
public:
    std::vector<DbEntry> entries;
    QString extraFileName;
    QStringList extras;
};

using MimeBinaryCache = QHash<QString, bool>;

QStringList filterFromFileName(const QStringList &flags, const QString &fileName);

void filteredFlags(const Utils::FilePath &filePath,
                   const Utils::FilePath &workingDir,
                   QStringList &flags,
                   QVector<ProjectExplorer::HeaderPath> &headerPaths,
                   QVector<ProjectExplorer::Macro> &macros,
                   CppEditor::ProjectFile::Kind &fileKind,
                   Utils::FilePath &sysRoot);

QStringList splitCommandLine(QString commandLine, QSet<QString> &flagsCache);

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
