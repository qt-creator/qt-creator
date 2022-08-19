// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/aspects.h>

namespace TextEditor { class SimpleCodeStylePreferences; }

namespace Nim {

class NimSettings : public Utils::AspectContainer
{
    Q_DECLARE_TR_FUNCTIONS(Nim::NimSettings)

public:
    NimSettings();
    ~NimSettings();

    Utils::StringAspect nimSuggestPath;

    static TextEditor::SimpleCodeStylePreferences *globalCodeStyle();

private:
    void InitializeCodeStyleSettings();
    void TerminateCodeStyleSettings();
};

class NimToolsSettingsPage final : public Core::IOptionsPage
{
public:
    explicit NimToolsSettingsPage(NimSettings *settings);
};

} // namespace Nim

