// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "presetsparser.h"
#include "utils/temporarydirectory.h"

#include <qtsupport/qtprojectimporter.h>

namespace CMakeProjectManager {

class CMakeTool;

namespace Internal {

struct DirectoryData;

class CMakeProjectImporter : public QtSupport::QtProjectImporter
{
public:
    CMakeProjectImporter(const Utils::FilePath &path, const Internal::PresetsData &presetsData);

    Utils::FilePaths importCandidates() final;

private:
    QList<void *> examineDirectory(const Utils::FilePath &importPath,
                                   QString *warningMessage) const final;
    bool matchKit(void *directoryData, const ProjectExplorer::Kit *k) const final;
    ProjectExplorer::Kit *createKit(void *directoryData) const final;
    const QList<ProjectExplorer::BuildInfo> buildInfoList(void *directoryData) const final;

    struct CMakeToolData {
        bool isTemporary = false;
        CMakeTool *cmakeTool = nullptr;
    };
    CMakeToolData findOrCreateCMakeTool(const Utils::FilePath &cmakeToolPath) const;

    void deleteDirectoryData(void *directoryData) const final;

    void cleanupTemporaryCMake(ProjectExplorer::Kit *k, const QVariantList &vl);
    void persistTemporaryCMake(ProjectExplorer::Kit *k, const QVariantList &vl);

    void ensureBuildDirectory(DirectoryData &data, const ProjectExplorer::Kit *k) const;

    Internal::PresetsData m_presetsData;
    Utils::TemporaryDirectory m_presetsTempDir;
};

} // namespace Internal
} // namespace CMakeProjectManager
