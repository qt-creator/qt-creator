// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ProjectExplorer {
class Project;
}

namespace CMakeProjectManager::Internal {

class CMakeSpecificSettings final : public Utils::AspectContainer
{
    ProjectExplorer::Project *project{nullptr};
public:
    CMakeSpecificSettings(ProjectExplorer::Project *project, bool autoApply);

    void readSettings() final;
    void writeSettings() const final;

    Utils::BoolAspect autorunCMake{this};
    Utils::FilePathAspect ninjaPath{this};
    Utils::BoolAspect packageManagerAutoSetup{this};
    Utils::BoolAspect askBeforeReConfigureInitialParams{this};
    Utils::BoolAspect askBeforePresetsReload{this};
    Utils::BoolAspect showSourceSubFolders{this};
    Utils::BoolAspect showAdvancedOptionsByDefault{this};
    Utils::BoolAspect useJunctionsForSourceAndBuildDirectories{this};

    bool useGlobalSettings{true};
};

CMakeSpecificSettings &settings(ProjectExplorer::Project *project);

} // CMakeProjectManager::Internal
