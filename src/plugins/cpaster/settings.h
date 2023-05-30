// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace CodePaster {

class Settings : public Core::PagedSettings
{
public:
    Settings();

    Utils::StringAspect username{this};
    Utils::SelectionAspect protocols{this};
    Utils::IntegerAspect expiryDays{this};
    Utils::BoolAspect copyToClipboard{this};
    Utils::BoolAspect displayOutput{this};
};

} // CodePaster
