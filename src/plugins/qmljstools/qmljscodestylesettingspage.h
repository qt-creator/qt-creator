// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljscodestylesettings.h"
#include "qmljsformatterselectionwidget.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <texteditor/codestyleeditor.h>

#include <QStackedWidget>

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
QT_END_NAMESPACE

namespace TextEditor {
    class FontSettings;
}

namespace QmlJSTools {
class QmlJSCodeStyleSettings;
namespace Internal {

class QmlJSCodeStylePreferencesWidget : public TextEditor::CodeStyleEditorWidget
{
    Q_OBJECT

public:
    explicit QmlJSCodeStylePreferencesWidget(const QString &previewText, QWidget *parent = nullptr);

    void setPreferences(QmlJSCodeStylePreferences* preferences);

private:
    void decorateEditor(const TextEditor::FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void slotSettingsChanged(const QmlJSCodeStyleSettings &);
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences);
    void updatePreview();
    void builtInFormatterPreview();
    void qmlformatPreview();
    void customFormatterPreview();

    FormatterSelectionWidget *m_formatterSelectionWidget;
    QStackedWidget *m_formatterSettingsStack;
    TextEditor::SnippetEditorWidget *m_previewTextEdit;
    QmlJSCodeStylePreferences *m_preferences = nullptr;
};

class QmlJSCodeStyleSettingsPage : public Core::IOptionsPage
{
public:
    QmlJSCodeStyleSettingsPage();
};

} // namespace Internal
} // namespace QmlJSTools
