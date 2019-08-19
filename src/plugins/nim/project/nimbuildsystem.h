/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/treescanner.h>

#include <utils/filesystemwatcher.h>

namespace Nim {

class NimBuildSystem : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    explicit NimBuildSystem(ProjectExplorer::Project *project);

    bool addFiles(const QStringList &filePaths);
    bool removeFiles(const QStringList &filePaths);
    bool renameFile(const QString &filePath, const QString &newFilePath);

    void setExcludedFiles(const QStringList &list); // Keep for compatibility with Qt Creator 4.10
    QStringList excludedFiles(); // Make private when no longer supporting Qt Creator 4.10

    void parseProject(ParsingContext &&ctx) final;

    const Utils::FilePathList nimFiles() const;

private:
    void loadSettings();
    void saveSettings();

    void collectProjectFiles();
    void updateProject();

    QStringList m_excludedFiles;

    ProjectExplorer::TreeScanner m_scanner;

    ParsingContext m_currentContext;

    Utils::FileSystemWatcher m_directoryWatcher;
};

} // namespace Nim
