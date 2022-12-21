// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nimbuildsystem.h"

namespace Nim {

struct NimbleTask
{
    QString name;
    QString description;

    bool operator==(const NimbleTask &o) const {
        return name == o.name && description == o.description;
    }
};

struct NimbleMetadata
{
    QStringList bin;
    QString binDir;
    QString srcDir;

    bool operator==(const NimbleMetadata &o) const {
        return bin == o.bin && binDir == o.binDir && srcDir == o.srcDir;
    }
    bool operator!=(const NimbleMetadata &o) const {
        return  !operator==(o);
    }
};

class NimbleBuildSystem : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    NimbleBuildSystem(ProjectExplorer::Target *target);

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
    bool renameFile(ProjectExplorer::Node *,
                    const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath) override;
    QString name() const final { return QLatin1String("mimble"); }
    void triggerParsing() final;

    std::vector<NimbleTask> m_tasks;

    NimProjectScanner m_projectScanner;
    ParseGuard m_guard;
};

} // Nim
