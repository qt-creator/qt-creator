// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsformatterselectionwidget.h"
#include "qmljstoolstr.h"
#include <utils/layoutbuilder.h>

namespace QmlJSTools {

QmlCodeStyleWidgetBase::QmlCodeStyleWidgetBase(QWidget *parent)
    : QWidget(parent)
{
}

FormatterSelectionWidget::FormatterSelectionWidget(QWidget *parent)
    : QmlCodeStyleWidgetBase(parent)
{
    using namespace Utils;
    m_formatterSelection.setDefaultValue(Builtin);
    m_formatterSelection.setDisplayStyle(SelectionAspect::DisplayStyle::RadioButtons);
    m_formatterSelection.addOption(Tr::tr("Built-In Formatter [Deprecated]") );
    m_formatterSelection.addOption(Tr::tr("QmlFormat [LSP]"));
    m_formatterSelection.addOption(Tr::tr("Custom Formatter [Must be qmlformat compatible]"));
    m_formatterSelection.setLabelText(Tr::tr("Formatter"));

    connect(&m_formatterSelection, &SelectionAspect::changed,
            this, &FormatterSelectionWidget::slotSettingsChanged);

    using namespace Layouting;
    Column {
        Group {
            title(Tr::tr("Formatter Selection")),
            Column {
                m_formatterSelection, br
            }
        }
    }.attachTo(this);
}

void FormatterSelectionWidget::setCodeStyleSettings(const QmlJSCodeStyleSettings &s)
{
    if (s.formatter != m_formatterSelection.value())
        m_formatterSelection.setValue(s.formatter);
}

void FormatterSelectionWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    if (m_preferences == preferences)
        return; // nothing changes

    slotCurrentPreferencesChanged(preferences);

    // cleanup old
    if (m_preferences) {
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, nullptr);
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                this, &FormatterSelectionWidget::slotCurrentPreferencesChanged);
    }
    m_preferences = preferences;
    // fillup new
    if (m_preferences) {
        setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        connect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, [this] {
            setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        });
        connect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                this, &FormatterSelectionWidget::slotCurrentPreferencesChanged);
    }
}

void FormatterSelectionWidget::slotCurrentPreferencesChanged(
    TextEditor::ICodeStylePreferences *preferences)
{
    QmlJSCodeStylePreferences *current = dynamic_cast<QmlJSCodeStylePreferences *>(
        preferences ? preferences->currentPreferences() : nullptr);
    const bool enableWidgets = current && !current->isReadOnly();
    setEnabled(enableWidgets);
}

void FormatterSelectionWidget::slotSettingsChanged()
{
    QmlJSCodeStyleSettings settings = m_preferences
                                          ? m_preferences->currentCodeStyleSettings()
                                          : QmlJSCodeStyleSettings::currentGlobalCodeStyle();
    settings.formatter = static_cast<QmlJSCodeStyleSettings::Formatter>(m_formatterSelection.value());
    emit settingsChanged(settings);
}

} // namespace QmlJSTools
