// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscodestylepreferenceswidget.h"

#include "qmljscodestylepreferences.h"
#include "qmljscodestylesettings.h"
#include "qmljscodestylesettingswidget.h"

#include <QLabel>
#include <QVBoxLayout>

namespace QmlJSTools {

QmlJSCodeStylePreferencesWidget::QmlJSCodeStylePreferencesWidget(QWidget *parent) :
      QWidget(parent)
{
    m_codeStyleSettingsWidget = new QmlJSCodeStyleSettingsWidget(this);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_codeStyleSettingsWidget);
    layout->setContentsMargins(QMargins());
}

void QmlJSCodeStylePreferencesWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    if (m_preferences == preferences)
        return; // nothing changes

    slotCurrentPreferencesChanged(preferences);

    // cleanup old
    if (m_preferences) {
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentCodeStyleSettingsChanged,
                   m_codeStyleSettingsWidget, &QmlJSCodeStyleSettingsWidget::setCodeStyleSettings);
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                   this, &QmlJSCodeStylePreferencesWidget::slotCurrentPreferencesChanged);
        disconnect(m_codeStyleSettingsWidget, &QmlJSCodeStyleSettingsWidget::settingsChanged,
                   this, &QmlJSCodeStylePreferencesWidget::slotSettingsChanged);
    }
    m_preferences = preferences;
    // fillup new
    if (m_preferences) {
        m_codeStyleSettingsWidget->setCodeStyleSettings(m_preferences->currentCodeStyleSettings());

        connect(m_preferences, &QmlJSCodeStylePreferences::currentCodeStyleSettingsChanged,
                m_codeStyleSettingsWidget, &QmlJSCodeStyleSettingsWidget::setCodeStyleSettings);
        connect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                this, &QmlJSCodeStylePreferencesWidget::slotCurrentPreferencesChanged);
        connect(m_codeStyleSettingsWidget, &QmlJSCodeStyleSettingsWidget::settingsChanged,
                this, &QmlJSCodeStylePreferencesWidget::slotSettingsChanged);
    }
}

void QmlJSCodeStylePreferencesWidget::slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences)
{
    m_codeStyleSettingsWidget->setEnabled(preferences && preferences->currentPreferences() &&
                                          !preferences->currentPreferences()->isReadOnly());
}

void QmlJSCodeStylePreferencesWidget::slotSettingsChanged(const QmlJSCodeStyleSettings &settings)
{
    if (!m_preferences)
        return;

    QmlJSCodeStylePreferences *current = qobject_cast<QmlJSCodeStylePreferences*>(m_preferences->currentPreferences());
    if (!current)
        return;

    current->setCodeStyleSettings(settings);
}

} // namespace QmlJSTools
