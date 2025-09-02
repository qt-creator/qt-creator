// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/treescanner.h>

#include <utils/filesystemwatcher.h>

namespace Nim {
class NimBuildSystem;

class NimBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

    friend class ProjectExplorer::BuildConfigurationFactory;
    NimBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);

public:
    Utils::FilePath cacheDirectory() const;
    Utils::FilePath outFilePath() const;
};

Utils::FilePath nimPathFromKit(ProjectExplorer::Kit *kit);
Utils::FilePath nimblePathFromKit(ProjectExplorer::Kit *kit);

class NimProjectScanner : public QObject
{
    Q_OBJECT

public:
    explicit NimProjectScanner(ProjectExplorer::Project *project);

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
    void directoryChanged(const Utils::FilePath &path);
    void fileChanged(const Utils::FilePath &path);

private:
    void loadSettings();
    void saveSettings();

    ProjectExplorer::Project *m_project = nullptr;
    ProjectExplorer::TreeScanner m_scanner;
    Utils::FileSystemWatcher m_directoryWatcher;
};

void setupNimProject();

} // Nim
