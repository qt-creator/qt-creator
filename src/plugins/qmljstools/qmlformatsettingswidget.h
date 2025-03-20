// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljsformatterselectionwidget.h"
#include "qmljscodestylesettings.h"

#include <texteditor/snippets/snippeteditor.h>

#include <memory>
#include <QWidget>

namespace QmlJSTools {
class FormatterSelectionWidget;

class QmlFormatSettingsWidget : public QmlCodeStyleWidgetBase
{
public:
    explicit QmlFormatSettingsWidget(
        QWidget *parent = nullptr,
        FormatterSelectionWidget *selection = nullptr);
    void setCodeStyleSettings(const QmlJSCodeStyleSettings &s) override;
    void setPreferences(QmlJSCodeStylePreferences *preferences) override;
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences* preferences) override;

private:
    void slotSettingsChanged();
    std::unique_ptr<TextEditor::SnippetEditorWidget> m_qmlformatConfigTextEdit;
    FormatterSelectionWidget *m_formatterSelectionWidget = nullptr;
    QmlJSCodeStylePreferences *m_preferences = nullptr;
};

} // namespace QmlJSTools
