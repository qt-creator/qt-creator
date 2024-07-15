// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildsystem.h>

namespace Python::Internal {

class PythonBuildConfiguration;

class PythonBuildSystem final : public ProjectExplorer::BuildSystem
{
public:
    explicit PythonBuildSystem(PythonBuildConfiguration *buildConfig);
    explicit PythonBuildSystem(ProjectExplorer::Target *target);

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
    bool renameFile(ProjectExplorer::Node *,
                    const Utils::FilePath &oldFilePath,
                    const Utils::FilePath &newFilePath) override;
    QString name() const override { return QLatin1String("python"); }

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

    QList<FileEntry> m_files;
    QList<FileEntry> m_qmlImportPaths;
    PythonBuildConfiguration *m_buildConfig = nullptr;
};


} // namespace Python::Internal
