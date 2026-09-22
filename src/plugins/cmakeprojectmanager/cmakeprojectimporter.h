// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/qtprojectimporter.h>

#include <utils/filepath.h>

namespace CMakeProjectManager {

class CMakeProject;

namespace Internal {

struct DirectoryData;

class CMakeProjectImporter : public QtSupport::QtProjectImporter
{
public:
    CMakeProjectImporter(const Utils::FilePath &path,
                         const CMakeProjectManager::CMakeProject *project);

    Utils::FilePaths importCandidates() final;
    ProjectExplorer::Target *preferredTarget(const QList<ProjectExplorer::Target *> &possibleTargets) final;
    bool filter(ProjectExplorer::Kit *k) const final;

    Utils::FilePaths presetCandidates();
    void createKitsFromPresets();

private:
    QList<void *> examineDirectory(const Utils::FilePath &importPath,
                                   QString *warningMessage) const final;
    bool matchKit(void *directoryData, const ProjectExplorer::Kit *k) const final;
    ProjectExplorer::Kit *createKit(void *directoryData) const final;
    ProjectExplorer::BuildInfo buildInfo(void *directoryData) const final;

    void deleteDirectoryData(void *directoryData) const final;

    void ensureBuildDirectory(const DirectoryData &data, const ProjectExplorer::Kit *k) const;

    void applyDirectoryDataToKit(const DirectoryData &data, ProjectExplorer::Kit *k) const;

    const CMakeProject *m_project;
    Utils::FilePath m_presetsProbeDir;
};

#ifdef WITH_TESTS
QObject *createCMakeProjectImporterTest();
#endif

} // namespace Internal
} // namespace CMakeProjectManager
