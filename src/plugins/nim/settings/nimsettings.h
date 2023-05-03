// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/aspects.h>

namespace TextEditor { class SimpleCodeStylePreferences; }

namespace Nim {

class NimSettings : public Utils::AspectContainer
{
public:
    NimSettings();
    ~NimSettings();

    Utils::StringAspect nimSuggestPath;

    static TextEditor::SimpleCodeStylePreferences *globalCodeStyle();
};

class NimToolsSettingsPage final : public Core::IOptionsPage
{
public:
    explicit NimToolsSettingsPage(NimSettings *settings);
};

} // Nim

