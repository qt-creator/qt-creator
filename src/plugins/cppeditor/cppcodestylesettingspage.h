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

namespace CppEditor {
class CppCodeStylePreferences;

class CPPEDITOR_EXPORT CppCodeStyleWidget : public TextEditor::CodeStyleEditorWidget
{
    Q_OBJECT
public:
    CppCodeStyleWidget(QWidget *parent = nullptr)
        : CodeStyleEditorWidget(parent)
    {}

    virtual void setCodeStyleSettings(const CppEditor::CppCodeStyleSettings &) {}
    virtual void setTabSettings(const TextEditor::TabSettings &) {}
    virtual void synchronize() {}

signals:
    void codeStyleSettingsChanged(const CppEditor::CppCodeStyleSettings &);
    void tabSettingsChanged(const TextEditor::TabSettings &);
};

namespace Internal {

class CppCodeStylePreferencesWidgetPrivate;

class CppCodeStylePreferencesWidget : public TextEditor::CodeStyleEditorWidget
{
    Q_OBJECT
public:
    explicit CppCodeStylePreferencesWidget(QWidget *parent = nullptr);
    ~CppCodeStylePreferencesWidget() override;

    void setCodeStyle(CppCodeStylePreferences *codeStylePreferences);
    void addTab(CppCodeStyleWidget *page, QString tabName);
    void apply() override;
    void finish() override;

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
signals:
    void codeStyleSettingsChanged(const CppEditor::CppCodeStyleSettings &);
    void tabSettingsChanged(const TextEditor::TabSettings &);
    void applyEmitted();
    void finishEmitted();
};


class CppCodeStyleSettingsPage : public Core::IOptionsPage
{
public:
    CppCodeStyleSettingsPage();
};

} // namespace Internal
} // namespace CppEditor
