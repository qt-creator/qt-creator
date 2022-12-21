// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macrooptionspage.h"

#include "macromanager.h"
#include "macrosconstants.h"
#include "macrooptionswidget.h"

#include <texteditor/texteditorconstants.h>

namespace Macros {
namespace Internal {

MacroOptionsPage::MacroOptionsPage()
{
    setId(Constants::M_OPTIONS_PAGE);
    setDisplayName(MacroOptionsWidget::tr("Macros"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new MacroOptionsWidget; });
}

} // Internal
} // Macros
