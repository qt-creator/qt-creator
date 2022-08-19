// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace CMakeProjectManager {
namespace Internal {

enum AfterAddFileAction : int {
    AskUser,
    CopyFilePath,
    NeverCopyFilePath
};

class CMakeSpecificSettings final : public Utils::AspectContainer
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::Internal::CMakeSpecificSettings)

public:
    CMakeSpecificSettings();

    Utils::SelectionAspect afterAddFileSetting;
    Utils::StringAspect ninjaPath;
    Utils::BoolAspect packageManagerAutoSetup;
    Utils::BoolAspect askBeforeReConfigureInitialParams;
    Utils::BoolAspect showSourceSubFolders;
};

class CMakeSpecificSettingsPage final : public Core::IOptionsPage
{
public:
    explicit CMakeSpecificSettingsPage(CMakeSpecificSettings *settings);
};

} // Internal
} // CMakeProjectManager
