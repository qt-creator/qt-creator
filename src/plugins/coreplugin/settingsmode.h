// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imode.h"

#include <utils/id.h>

namespace Core::Internal {

class SettingsModeWidget;

class SettingsMode final : public IMode
{
public:
    SettingsMode();
    ~SettingsMode() final;

    void open(Utils::Id initialPage);

private:
    SettingsModeWidget *m_settingsModeWidget = nullptr;
};

} // namespace Core::Internal
