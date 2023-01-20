// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace CMakeProjectManager::Internal {

enum AfterAddFileAction : int {
    AskUser,
    CopyFilePath,
    NeverCopyFilePath
};

class CMakeSpecificSettings final : public Utils::AspectContainer
{
public:
    CMakeSpecificSettings();

    static CMakeSpecificSettings *instance();

    Utils::BoolAspect autorunCMake;
    Utils::SelectionAspect afterAddFileSetting;
    Utils::StringAspect ninjaPath;
    Utils::BoolAspect packageManagerAutoSetup;
    Utils::BoolAspect askBeforeReConfigureInitialParams;
    Utils::BoolAspect showSourceSubFolders;
    Utils::BoolAspect showAdvancedOptionsByDefault;
};

class CMakeSpecificSettingsPage final : public Core::IOptionsPage
{
public:
    CMakeSpecificSettingsPage();
};

} // CMakeProjectManager::Internal
