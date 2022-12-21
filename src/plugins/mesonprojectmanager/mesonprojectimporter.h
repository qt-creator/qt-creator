// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>

#include <qtsupport/qtprojectimporter.h>

namespace MesonProjectManager::Internal {

class MesonProjectImporter final : public QtSupport::QtProjectImporter
{
public:
    MesonProjectImporter(const Utils::FilePath &path);
    Utils::FilePaths importCandidates() final;

private:
    // importPath is an existing directory at this point!
    QList<void *> examineDirectory(const Utils::FilePath &importPath, QString *warningMessage) const final;
    // will get one of the results from examineDirectory
    bool matchKit(void *directoryData, const ProjectExplorer::Kit *k) const final;
    // will get one of the results from examineDirectory
    ProjectExplorer::Kit *createKit(void *directoryData) const final;
    // will get one of the results from examineDirectory
    const QList<ProjectExplorer::BuildInfo> buildInfoList(void *directoryData) const final;

    void deleteDirectoryData(void *directoryData) const final;
};

} // MesonProjectManager::Internal
