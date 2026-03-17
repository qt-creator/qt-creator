// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include "envvarseparatoraspect.h"

#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/terminalcommand.h>

namespace Core::Internal {

const char kEnvironmentChanges[] = "Core/EnvironmentChanges";
const char kEnvVarSeparators[] = "Core/EnvVarSeparators";

class EnvChangeAspect : public Utils::EnvironmentChangesAspect
{
public:
    using EnvironmentChangesAspect::EnvironmentChangesAspect;
    void addToLayoutImpl(Layouting::Layout &parent);
};

class CORE_TEST_EXPORT SystemSettings final : public Utils::AspectContainer
{
public:
    SystemSettings();

    Utils::BoolAspect useDbusFileManagers{this};
    Utils::StringAspect externalFileBrowser{this};

    Utils::FilePathAspect patchCommand{this};

    Utils::BoolAspect autoSaveModifiedFiles{this};
    Utils::IntegerAspect autoSaveInterval{this};

    Utils::BoolAspect autoSaveAfterRefactoring{this};

    Utils::BoolAspect autoSuspendEnabled{this};
    Utils::IntegerAspect autoSuspendMinDocumentCount{this};

    Utils::BoolAspect warnBeforeOpeningBigFiles{this};
    Utils::IntegerAspect bigFileSizeLimitInMB{this};

    Utils::IntegerAspect maxRecentFiles{this};

    Utils::SelectionAspect reloadSetting{this};

    Utils::BoolAspect askBeforeExit{this};

    EnvChangeAspect environmentChangesAspect{this};
    EnvVarSeparatorAspect envVarSeparatorAspect{this};

    Utils::TerminalCommandAspect terminalCommand{this};

    Utils::BoolAspect enableCrashReports{this};

    void delayedInitialize();
};

CORE_TEST_EXPORT SystemSettings &systemSettings();

} // namespace Core::Internal
