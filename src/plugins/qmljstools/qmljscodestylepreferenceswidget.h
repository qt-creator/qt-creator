// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljscodestylesettings.h"
#include "qmljsformatterselectionwidget.h"

#include <texteditor/tabsettingswidget.h>
#include <utils/aspects.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QSpinBox;
QT_END_NAMESPACE
namespace QmlJSTools {

class BuiltinFormatterSettingsWidget : public QmlCodeStyleWidgetBase
{
public:
    explicit BuiltinFormatterSettingsWidget(QWidget *parent, FormatterSelectionWidget *selection);
    void setCodeStyleSettings(const QmlJSCodeStyleSettings &settings) override;
    void setPreferences(QmlJSCodeStylePreferences *tabPreferences) override;
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences* preferences) override;

private:
    void slotSettingsChanged();
    void slotTabSettingsChanged(const TextEditor::TabSettings &settings);

    Utils::IntegerAspect m_lineLength;
    TextEditor::TabSettingsWidget *m_tabSettingsWidget;
    QmlJSCodeStylePreferences *m_preferences = nullptr;
    FormatterSelectionWidget *m_formatterSelectionWidget;
};

} // namespace QmlJSTools
