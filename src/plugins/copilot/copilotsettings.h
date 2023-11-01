// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ProjectExplorer { class Project; }

namespace Copilot {

class CopilotSettings : public Utils::AspectContainer
{
public:
    CopilotSettings();

    Utils::FilePathAspect nodeJsPath{this};
    Utils::FilePathAspect distPath{this};
    Utils::BoolAspect autoComplete{this};
    Utils::BoolAspect enableCopilot{this};

    Utils::BoolAspect useProxy{this};
    Utils::StringAspect proxyHost{this};
    Utils::IntegerAspect proxyPort{this};
    Utils::StringAspect proxyUser{this};

    Utils::BoolAspect saveProxyPassword{this};
    Utils::StringAspect proxyPassword{this};
    Utils::BoolAspect proxyRejectUnauthorized{this};
};

CopilotSettings &settings();

class CopilotProjectSettings : public Utils::AspectContainer
{
public:
    explicit CopilotProjectSettings(ProjectExplorer::Project *project);

    void save(ProjectExplorer::Project *project);
    void setUseGlobalSettings(bool useGlobalSettings);

    bool isEnabled() const;

    Utils::BoolAspect enableCopilot{this};
    Utils::BoolAspect useGlobalSettings{this};
};

} // namespace Copilot
