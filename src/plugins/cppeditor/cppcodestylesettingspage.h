// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppcodestylesettings.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <texteditor/codestyleeditor.h>
#include <texteditor/tabsettings.h>

#include <QWidget>
#include <QPointer>

namespace TextEditor {
    class FontSettingsData;
    class SnippetEditorWidget;
    class CodeStyleEditor;
}

namespace CppEditor {

namespace Internal {

class CppCodeStylePreferencesWidgetPrivate;

void setupCppCodeStyleSettings();

} // namespace Internal

class CPPEDITOR_EXPORT CppCodeStylePreferencesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CppCodeStylePreferencesWidget(QWidget *parent = nullptr);
    ~CppCodeStylePreferencesWidget() override;

    void setCodeStyle(CppCodeStylePreferences *codeStylePreferences);

    void apply();
    void finish();

signals:
    void applyEmitted();
    void finishEmitted();

private:
    void decorateEditors(const TextEditor::FontSettingsData &fontSettings);
    void setVisualizeWhitespace(bool on);
    void slotTabSettingsChanged(const TextEditor::TabSettingsData &settings);
    void slotCodeStyleSettingsChanged();
    void updatePreview();
    void setTabSettings(const TextEditor::TabSettingsData &settings);
    TextEditor::TabSettingsData tabSettings() const;
    void setCodeStyleSettings(const CppCodeStyleSettings &settings, bool preview = true);
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *, bool preview = true);

    CppCodeStyleSettings cppCodeStyleSettings() const;

    CppCodeStylePreferences *m_preferences = nullptr;
    Internal::CppCodeStylePreferencesWidgetPrivate *d = nullptr;
    CppCodeStyleSettings m_originalCppCodeStyleSettings;
    TextEditor::TabSettingsData m_originalTabSettings;
    bool m_blockUpdates = false;

    friend class Internal::CppCodeStylePreferencesWidgetPrivate;
};

} // namespace CppEditor
