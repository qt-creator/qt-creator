// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace TextEditor { class SimpleCodeStylePreferences; }

namespace Nim {

class NimSettings : public Core::PagedSettings
{
public:
    NimSettings();
    ~NimSettings();

    Utils::FilePathAspect nimSuggestPath{this};

    static TextEditor::SimpleCodeStylePreferences *globalCodeStyle();
};

} // Nim

