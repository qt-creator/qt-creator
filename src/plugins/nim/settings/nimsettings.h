// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace TextEditor { class SimpleCodeStylePreferences; }

namespace Nim {

class NimSettings final : public Utils::AspectContainer
{
public:
    NimSettings();
    ~NimSettings();

    Utils::FilePathAspect nimSuggestPath{this};

    static TextEditor::SimpleCodeStylePreferences *globalCodeStyle();
};

NimSettings &settings();

} // Nim

