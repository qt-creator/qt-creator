// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ProjectExplorer { class Project; }

namespace Copilot::Internal {

class CopilotSettings final : public Utils::AspectContainer
{
public:
    CopilotSettings();

    Utils::FilePathAspect nodeJsPath{this};
    Utils::FilePathAspect distPath{this};
    Utils::BoolAspect autoComplete{this};
    Utils::BoolAspect enableCopilot{this};

    Utils::StringAspect proxy{this};
    Utils::BoolAspect proxyRejectUnauthorized{this};

    Utils::StringAspect githubEnterpriseUrl{this};
};

CopilotSettings &settings();

bool isCopilotEnabled(ProjectExplorer::Project *project);

void setupCopilotSettings();

} // namespace Copilot::Internal

