// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor.h>

namespace Nim {

class NimEditorFactory final : public TextEditor::TextEditorFactory
{
public:
    NimEditorFactory();

    static void decorateEditor(TextEditor::TextEditorWidget *editor);
};

} // Nim
