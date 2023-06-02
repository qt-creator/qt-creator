// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ProjectExplorer {
class Project;
}

namespace Copilot {

class CopilotSettings : public Utils::AspectContainer
{
public:
    CopilotSettings();

    static CopilotSettings &instance();

    Utils::FilePathAspect nodeJsPath{this};
    Utils::FilePathAspect distPath{this};
    Utils::BoolAspect autoComplete{this};
    Utils::BoolAspect enableCopilot{this};
};

class CopilotProjectSettings : public Utils::AspectContainer
{
public:
    CopilotProjectSettings(ProjectExplorer::Project *project, QObject *parent = nullptr);

    void save(ProjectExplorer::Project *project);
    void setUseGlobalSettings(bool useGlobalSettings);

    bool isEnabled() const;

    Utils::BoolAspect enableCopilot{this};
    Utils::BoolAspect useGlobalSettings{this};
};

} // namespace Copilot
