// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/icodestylepreferencesfactory.h>

namespace TextEditor {
class FontSettings;
class SnippetEditorWidget;
} // TextEditor

namespace Nim {

class NimCodeStylePreferencesWidget : public TextEditor::CodeStyleEditorWidget
{
    Q_OBJECT

public:
    NimCodeStylePreferencesWidget(TextEditor::ICodeStylePreferences *preferences, QWidget *parent = nullptr);
    ~NimCodeStylePreferencesWidget();

private:
    void decorateEditor(const TextEditor::FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void updatePreview();

    TextEditor::ICodeStylePreferences *m_preferences;
    TextEditor::SnippetEditorWidget *m_previewTextEdit;
};

} // Nim
