// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace ClearCase::Internal {

class ClearCaseSettingsPage final : public Core::IOptionsPage
{
public:
    ClearCaseSettingsPage();
};

} // ClearCase::Internal
