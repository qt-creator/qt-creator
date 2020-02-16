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

class NimProjectScanner : public QObject
{
    Q_OBJECT

public:
    explicit NimProjectScanner(ProjectExplorer::Project *project);

    void setFilter(const ProjectExplorer::TreeScanner::FileFilter &filter);
    void startScan();
    void watchProjectFilePath();

    void setExcludedFiles(const QStringList &list);
    QStringList excludedFiles() const;

    bool addFiles(const QStringList &filePaths);
    ProjectExplorer::RemovedFilesFromProject removeFiles(const QStringList &filePaths);
    bool renameFile(const QString &from, const QString &to);

signals:
    void finished();
    void requestReparse();
    void directoryChanged(const QString &path);
    void fileChanged(const QString &path);

private:
    void loadSettings();
    void saveSettings();

    ProjectExplorer::Project *m_project = nullptr;
    ProjectExplorer::TreeScanner m_scanner;
    Utils::FileSystemWatcher m_directoryWatcher;
};

class NimBuildSystem : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    explicit NimBuildSystem(ProjectExplorer::Target *target);

    bool supportsAction(ProjectExplorer::Node *,
                        ProjectExplorer::ProjectAction action,
                        const ProjectExplorer::Node *node) const final;
    bool addFiles(ProjectExplorer::Node *node,
                  const QStringList &filePaths, QStringList *) final;
    ProjectExplorer::RemovedFilesFromProject removeFiles(ProjectExplorer::Node *node,
                                                         const QStringList &filePaths,
                                                         QStringList *) override;
    bool deleteFiles(ProjectExplorer::Node *, const QStringList &) final;
    bool renameFile(ProjectExplorer::Node *,
                    const QString &filePath, const QString &newFilePath) final;

    void triggerParsing() override;

protected:
    void loadSettings();
    void saveSettings();

    void collectProjectFiles();

    ParseGuard m_guard;
    NimProjectScanner m_projectScanner;
};

} // namespace Nim
