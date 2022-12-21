// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace Autotest {
namespace Internal {

class GTestSettings : public Utils::AspectContainer
{
public:
    GTestSettings();

    Utils::IntegerAspect iterations;
    Utils::IntegerAspect seed;
    Utils::BoolAspect runDisabled;
    Utils::BoolAspect shuffle;
    Utils::BoolAspect repeat;
    Utils::BoolAspect throwOnFailure;
    Utils::BoolAspect breakOnFailure;
    Utils::SelectionAspect groupMode;
    Utils::StringAspect gtestFilter;
};

class GTestSettingsPage final : public Core::IOptionsPage
{
public:
    GTestSettingsPage(GTestSettings *settings, Utils::Id settingsId);
};

} // namespace Internal
} // namespace Autotest
