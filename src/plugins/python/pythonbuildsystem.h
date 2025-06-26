// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildsystem.h>

namespace Python::Internal {

class PythonBuildSystem final : public ProjectExplorer::BuildSystem
{
public:
    explicit PythonBuildSystem(ProjectExplorer::BuildConfiguration *buildConfig);

    bool supportsAction(ProjectExplorer::Node *context,
                        ProjectExplorer::ProjectAction action,
                        const ProjectExplorer::Node *node) const override;
    bool addFiles(ProjectExplorer::Node *,
                  const Utils::FilePaths &filePaths,
                  Utils::FilePaths *) override;
    ProjectExplorer::RemovedFilesFromProject removeFiles(ProjectExplorer::Node *,
                                                         const Utils::FilePaths &filePaths,
                                                         Utils::FilePaths *) override;
    bool deleteFiles(ProjectExplorer::Node *, const Utils::FilePaths &) override;
    bool renameFiles(
        ProjectExplorer::Node *,
        const Utils::FilePairs &filesToRename,
        Utils::FilePaths *notRenamed) override;

    void parse();
    bool save();

    bool writePyProjectFile(const Utils::FilePath &filePath, QString &content,
                            const QStringList &rawList, QString *errorMessage);

    void triggerParsing() final;

private:
    struct FileEntry {
        QString rawEntry;
        Utils::FilePath filePath;
    };
    QList<FileEntry> processEntries(const QStringList &paths) const;

    void updateQmlCodeModelInfo(ProjectExplorer::QmlCodeModelInfo &projectInfo) final;

    QList<FileEntry> m_files;
    QList<FileEntry> m_qmlImportPaths;
};

} // namespace Python::Internal
