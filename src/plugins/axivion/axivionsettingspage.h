// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Axivion::Internal {

class AxivionSettings;

class AxivionSettingsPage : public Core::IOptionsPage
{
public:
    explicit AxivionSettingsPage(AxivionSettings *settings);

private:
    AxivionSettings *m_settings;
};

} // Axivion::Internal
