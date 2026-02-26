// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gnproject.h"
#include "gnprojectparser.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/target.h>

#include <utils/filesystemwatcher.h>

namespace ProjectExplorer {

class ProjectUpdater;

}

namespace GNProjectManager::Internal {

class GNBuildSystem final : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    GNBuildSystem(ProjectExplorer::BuildConfiguration *bc);
    ~GNBuildSystem() final;

    static QString name() { return "gn"; }
    void triggerParsing() final;

    inline const GNTargetsList &targets() const { return m_parser.targets(); }

    bool generate();
    Utils::FilePath argsPath() const;

    const QStringList &targetList() const { return m_parser.targetsNames(); }

    GNProject *project() const;

private:
    void parsingCompleted(bool success);
    ProjectExplorer::BuildSystem::ParseGuard m_parseGuard;
    GNProjectParser m_parser;
    std::unique_ptr<ProjectExplorer::ProjectUpdater> m_cppCodeModelUpdater;
    Utils::FileSystemWatcher m_argsWatcher;
};

void setupGNBuildSystem();

} // namespace GNProjectManager::Internal
