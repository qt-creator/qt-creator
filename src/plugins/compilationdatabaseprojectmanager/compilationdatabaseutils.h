// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/cppprojectfile.h>
#include <utils/filepath.h>

#include <QHash>
#include <QStringList>

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
    QString workingDir;
};

class DbContents {
public:
    std::vector<DbEntry> entries;
    QString extraFileName;
    QStringList extras;
};

using MimeBinaryCache = QHash<QString, bool>;

QStringList filterFromFileName(const QStringList &flags, QString baseName);

void filteredFlags(const QString &fileName,
                   const QString &workingDir,
                   QStringList &flags,
                   QVector<ProjectExplorer::HeaderPath> &headerPaths,
                   QVector<ProjectExplorer::Macro> &macros,
                   CppEditor::ProjectFile::Kind &fileKind,
                   Utils::FilePath &sysRoot);

QStringList splitCommandLine(QString commandLine, QSet<QString> &flagsCache);

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
