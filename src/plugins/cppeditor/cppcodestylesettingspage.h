// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppcodestylesettings.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/tabsettings.h>

#include <QWidget>
#include <QPointer>

namespace TextEditor {
    class FontSettings;
    class SnippetEditorWidget;
    class CodeStyleEditor;
    class CodeStyleEditorWidget;
}

namespace CppEditor::Internal {

class CppCodeStylePreferencesWidgetPrivate;

class CppCodeStylePreferencesWidget : public TextEditor::CodeStyleEditorWidget
{
    Q_OBJECT

public:
    explicit CppCodeStylePreferencesWidget(QWidget *parent = nullptr);
    ~CppCodeStylePreferencesWidget() override;

    void setCodeStyle(CppCodeStylePreferences *codeStylePreferences);
    void addTab(TextEditor::CodeStyleEditorWidget *page, QString tabName);
    void apply() override;
    void finish() override;

signals:
    void applyEmitted();
    void finishEmitted();

private:
    void decorateEditors(const TextEditor::FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void slotTabSettingsChanged(const TextEditor::TabSettings &settings);
    void slotCodeStyleSettingsChanged();
    void updatePreview();
    void setTabSettings(const TextEditor::TabSettings &settings);
    TextEditor::TabSettings tabSettings() const;
    void setCodeStyleSettings(const CppCodeStyleSettings &settings, bool preview = true);
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *, bool preview = true);

    CppCodeStyleSettings cppCodeStyleSettings() const;

    CppCodeStylePreferences *m_preferences = nullptr;
    CppCodeStylePreferencesWidgetPrivate *d = nullptr;
    CppCodeStyleSettings m_originalCppCodeStyleSettings;
    TextEditor::TabSettings m_originalTabSettings;
    bool m_blockUpdates = false;
    friend class CppCodeStylePreferencesWidgetPrivate;
};

void setupCppCodeStyleSettings();

} // namespace CppEditor::Internal
