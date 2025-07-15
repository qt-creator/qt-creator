// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nimproject.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>

namespace Nim {

struct NimbleTask
{
    QString name;
    QString description;

    bool operator==(const NimbleTask &o) const {
        return name == o.name && description == o.description;
    }
};

class NimbleBuildSystem final : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    NimbleBuildSystem(ProjectExplorer::BuildConfiguration *bc);
    ~NimbleBuildSystem();

    std::vector<NimbleTask> tasks() const;

signals:
    void tasksChanged();

private:
    void loadSettings();
    void saveSettings();

    void updateProject();

    bool supportsAction(ProjectExplorer::Node *,
                        ProjectExplorer::ProjectAction action,
                        const ProjectExplorer::Node *node) const override;
    bool addFiles(ProjectExplorer::Node *node,
                  const Utils::FilePaths &filePaths, Utils::FilePaths *) override;
    ProjectExplorer::RemovedFilesFromProject removeFiles(ProjectExplorer::Node *node,
                                                         const Utils::FilePaths &filePaths,
                                                         Utils::FilePaths *) override;
    bool deleteFiles(ProjectExplorer::Node *, const Utils::FilePaths &) override;
    bool renameFiles(
        ProjectExplorer::Node *,
        const Utils::FilePairs &filesToRename,
        Utils::FilePaths *notRenamed) override;

    void triggerParsing() final;

    std::vector<NimbleTask> m_tasks;

    NimProjectScanner m_projectScanner;
    ParseGuard m_guard;
};

class NimbleProject final : public ProjectExplorer::Project
{
public:
    NimbleProject(const Utils::FilePath &filename);

    // Keep for compatibility with Qt Creator 4.10
    void toMap(Utils::Store &map) const final;

    QStringList excludedFiles() const;
    void setExcludedFiles(const QStringList &excludedFiles);

protected:
    // Keep for compatibility with Qt Creator 4.10
    RestoreResult fromMap(const Utils::Store &map, QString *errorMessage) final;

    QStringList m_excludedFiles;
};

void setupNimbleProject();

} // Nim
