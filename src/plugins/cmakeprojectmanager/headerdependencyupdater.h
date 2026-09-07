// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "headerdependencies.h"
#include "headerdependencyscanner.h"

#include <projectexplorer/rawprojectpart.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <functional>

namespace CMakeProjectManager::Internal {

QStringList scanArguments(const ProjectExplorer::RawProjectPartFlags &flags,
                          const ProjectExplorer::Macros &macros,
                          const ProjectExplorer::HeaderPaths &headerPaths,
                          DependencyDialect dialect);

HeaderScanUnits planHeaderScan(const ProjectExplorer::RawProjectParts &parts,
                               const ProjectExplorer::ToolchainInfo &cToolchain,
                               const ProjectExplorer::ToolchainInfo &cxxToolchain);

ProjectExplorer::RawProjectParts withDiscoveredHeaders(
    const ProjectExplorer::RawProjectParts &parts,
    const HeaderDependencyStore &store,
    const Utils::FilePath &sourceDirectory,
    const Utils::FilePath &buildDirectory);

class HeaderDependencyUpdater
{
public:
    using Handler = std::function<void(const ProjectExplorer::RawProjectParts &parts,
                                       bool storedResultsChanged)>;

    void setStoreFile(const Utils::FilePath &storeFile);
    void setProjectDirectories(const Utils::FilePath &sourceDirectory,
                               const Utils::FilePath &buildDirectory);
    void setShowIncludesPrefix(const QString &prefix);

    void update(const ProjectExplorer::ProjectUpdateInfo &info,
                const Utils::Environment &environment,
                const Handler &handler);
    void cancel();

    QHash<Utils::FilePath, Utils::FilePaths> projectHeaderMap();

private:
    void loadStore();
    Utils::FilePath m_storeFile;
    Utils::FilePath m_sourceDirectory;
    Utils::FilePath m_buildDirectory;
    QString m_showIncludesPrefix;
    HeaderDependencyStore m_store;
    bool m_storeLoaded = false;
    QtTaskTree::QSingleTaskTreeRunner m_taskTreeRunner;
};

} // namespace CMakeProjectManager::Internal
