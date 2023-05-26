// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Autotest::Internal {

class GTestSettings : public Core::PagedSettings
{
public:
    explicit GTestSettings(Utils::Id settingsId);

    Utils::IntegerAspect iterations{this};
    Utils::IntegerAspect seed{this};
    Utils::BoolAspect runDisabled{this};
    Utils::BoolAspect shuffle{this};
    Utils::BoolAspect repeat{this};
    Utils::BoolAspect throwOnFailure{this};
    Utils::BoolAspect breakOnFailure{this};
    Utils::SelectionAspect groupMode{this};
    Utils::StringAspect gtestFilter{this};
};

} // Autotest::Internal
