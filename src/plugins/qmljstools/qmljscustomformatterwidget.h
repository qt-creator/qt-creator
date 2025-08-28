// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljscodestylesettings.h"
#include "qmljsformatterselectionwidget.h"

#include <utils/aspects.h>
namespace QmlJSTools {
class CustomFormatterWidget : public QmlCodeStyleWidgetBase
{
public:
    explicit CustomFormatterWidget(QWidget *parent, FormatterSelectionWidget *selection = nullptr);
    void setCodeStyleSettings(const QmlJSCodeStyleSettings &s) override;
    void setPreferences(QmlJSCodeStylePreferences *preferences) override;
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences* preferences) override;

private:
    void slotSettingsChanged();
    Utils::FilePathAspect m_customFormatterPath;
    Utils::StringAspect m_customFormatterArguments;
    FormatterSelectionWidget *m_formatterSelectionWidget = nullptr;
    QmlJSCodeStylePreferences *m_preferences = nullptr;
};

} // namespace QmlJSTools
