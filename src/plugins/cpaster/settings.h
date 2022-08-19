// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace CodePaster {

class Settings : public Utils::AspectContainer
{
    Q_DECLARE_TR_FUNCTIONS(CodePaster::Settings)

public:
    Settings();

    Utils::StringAspect username;
    Utils::SelectionAspect protocols;
    Utils::IntegerAspect expiryDays;
    Utils::BoolAspect copyToClipboard;
    Utils::BoolAspect displayOutput;
};

class SettingsPage final : public Core::IOptionsPage
{
public:
    SettingsPage(Settings *settings);
};

} // namespace CodePaster
