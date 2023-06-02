// Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfixsettingspage.h"

#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppquickfixsettingswidget.h"

namespace CppEditor::Internal {

CppQuickFixSettingsPage::CppQuickFixSettingsPage()
{
    setId(Constants::QUICK_FIX_SETTINGS_ID);
    setDisplayName(Tr::tr(Constants::QUICK_FIX_SETTINGS_DISPLAY_NAME));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new CppQuickFixSettingsWidget; });
}

} // CppEditor::Internal
