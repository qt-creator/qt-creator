// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Autotest::Internal {

class GTestSettings : public Core::PagedSettings
{
public:
    explicit GTestSettings(Utils::Id settingsId);

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

} // Autotest::Internal
