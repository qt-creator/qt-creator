// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscustomformatterwidget.h"
#include "qmlformatsettings.h"
#include "qmljsformatterselectionwidget.h"
#include "qmljstoolstr.h"

#include <utils/layoutbuilder.h>

#include <QWidget>

namespace QmlJSTools {

CustomFormatterWidget::CustomFormatterWidget(QWidget *parent, FormatterSelectionWidget *selection)
    : QmlCodeStyleWidgetBase(parent)
    , m_formatterSelectionWidget(selection)
{

    m_customFormatterPath.setParent(this);
    m_customFormatterArguments.setParent(this);

    m_customFormatterPath.setPlaceHolderText(
        QmlFormatSettings::instance().latestQmlFormatPath().toUrlishString());
    m_customFormatterPath.setLabelText(Tr::tr("Command:"));

    m_customFormatterArguments.setLabelText(Tr::tr("Arguments:"));
    m_customFormatterArguments.setDisplayStyle(Utils::StringAspect::LineEditDisplay);

    using namespace Layouting;
    // clang-format off
    Column {
        Group {
            title(Tr::tr("Custom Formatter Configuration")),
            Column {
                m_customFormatterPath, br,
                m_customFormatterArguments, br,
                st
            },
        },
        noMargin,
    }.attachTo(this);

    // clang-format on
    connect(
        &m_customFormatterPath,
        &Utils::BaseAspect::changed,
        this,
        &CustomFormatterWidget::slotSettingsChanged);
    connect(
        &m_customFormatterArguments,
        &Utils::BaseAspect::changed,
        this,
        &CustomFormatterWidget::slotSettingsChanged);
}

void CustomFormatterWidget::setCodeStyleSettings(const QmlJSCodeStyleSettings& s)
{
    QSignalBlocker blocker(this);
    if (s.customFormatterPath != m_customFormatterPath.expandedValue()) {
        m_customFormatterPath.setValue(s.customFormatterPath);
    }
    if (s.customFormatterArguments != m_customFormatterArguments.value()) {
        m_customFormatterArguments.setValue(s.customFormatterArguments);
    }
}

void CustomFormatterWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    if (m_preferences == preferences)
        return; // nothing changes

    slotCurrentPreferencesChanged(preferences);

    // cleanup old
    if (m_preferences) {
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, nullptr);
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                   this, &CustomFormatterWidget::slotCurrentPreferencesChanged);
    }
    m_preferences = preferences;
    // fillup new
    if (m_preferences) {
        setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        connect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, [this] {
                this->setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        });
        connect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                this, &CustomFormatterWidget::slotCurrentPreferencesChanged);
    }
}

void CustomFormatterWidget::slotCurrentPreferencesChanged(
    TextEditor::ICodeStylePreferences *preferences)
{
    QmlJSCodeStylePreferences *current = dynamic_cast<QmlJSCodeStylePreferences *>(
        preferences ? preferences->currentPreferences() : nullptr);
    const bool enableWidgets = current && !current->isReadOnly() && m_formatterSelectionWidget
                               && m_formatterSelectionWidget->selection().value()
                                      == QmlCodeStyleWidgetBase::Custom;
    setEnabled(enableWidgets);
}

void CustomFormatterWidget::slotSettingsChanged()
{
    QmlJSCodeStyleSettings settings = m_preferences ? m_preferences->currentCodeStyleSettings()
    : QmlJSCodeStyleSettings::currentGlobalCodeStyle();
    if (m_customFormatterPath.value().isEmpty()) {
        m_customFormatterPath.setValue(
            QmlFormatSettings::instance().latestQmlFormatPath().toUrlishString());
    }
    settings.customFormatterPath = m_customFormatterPath.expandedValue();
    settings.customFormatterArguments = m_customFormatterArguments.value();
    emit settingsChanged(settings);
}

} // namespace QmlJSTools
