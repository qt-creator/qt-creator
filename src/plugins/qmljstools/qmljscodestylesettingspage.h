// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <texteditor/icodestylepreferencesfactory.h>

#include <QWidget>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {
    class FontSettings;
    class CodeStyleEditor;
}

namespace QmlJSTools {
class QmlJSCodeStylePreferences;
class QmlJSCodeStyleSettings;

namespace Internal {

namespace Ui { class QmlJSCodeStyleSettingsPage; }

class QmlJSCodeStylePreferencesWidget : public TextEditor::CodeStyleEditorWidget
{
    Q_OBJECT

public:
    explicit QmlJSCodeStylePreferencesWidget(QWidget *parent = nullptr);
    ~QmlJSCodeStylePreferencesWidget() override;

    void setPreferences(QmlJSCodeStylePreferences* preferences);

private:
    void decorateEditor(const TextEditor::FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void slotSettingsChanged();
    void updatePreview();

    QmlJSCodeStylePreferences *m_preferences = nullptr;
    Ui::QmlJSCodeStyleSettingsPage *m_ui;
};


class QmlJSCodeStyleSettingsPage : public Core::IOptionsPage
{
public:
    QmlJSCodeStyleSettingsPage();

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    QmlJSCodeStylePreferences *m_preferences = nullptr;
    QPointer<TextEditor::CodeStyleEditor> m_widget;
};

} // namespace Internal
} // namespace QmlJSTools
