// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace CMakeProjectManager::Internal {

class CMakeSpecificSettings final : public Core::PagedSettings
{
public:
    CMakeSpecificSettings();

    static CMakeSpecificSettings *instance();

    Utils::BoolAspect autorunCMake;
    Utils::StringAspect ninjaPath;
    Utils::BoolAspect packageManagerAutoSetup;
    Utils::BoolAspect askBeforeReConfigureInitialParams;
    Utils::BoolAspect showSourceSubFolders;
    Utils::BoolAspect showAdvancedOptionsByDefault;
};

} // CMakeProjectManager::Internal
