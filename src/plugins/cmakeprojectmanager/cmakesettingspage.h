// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace CMakeProjectManager::Internal {

class CMakeSettingsPage final : public Core::IOptionsPage
{
public:
    CMakeSettingsPage();
};

} // CMakeProjectManager::Internal
