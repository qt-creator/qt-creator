// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Copilot {

class CopilotSettings : public Utils::AspectContainer
{
public:
    CopilotSettings();

    static CopilotSettings &instance();

    Utils::FilePathAspect nodeJsPath;
    Utils::FilePathAspect distPath;
    Utils::BoolAspect autoComplete;
};

} // namespace Copilot
