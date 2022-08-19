// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "simplecodestylepreferenceswidget.h"
#include "icodestylepreferences.h"
#include "tabsettings.h"
#include "tabsettingswidget.h"

#include <QVBoxLayout>

namespace TextEditor {

SimpleCodeStylePreferencesWidget::SimpleCodeStylePreferencesWidget(QWidget *parent) :
    QWidget(parent)
{
    m_tabSettingsWidget = new TabSettingsWidget(this);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_tabSettingsWidget);
    layout->setContentsMargins(QMargins());
    m_tabSettingsWidget->setEnabled(false);
}

void SimpleCodeStylePreferencesWidget::setPreferences(ICodeStylePreferences *preferences)
{
    if (m_preferences == preferences)
        return; // nothing changes

    // cleanup old
    if (m_preferences) {
        disconnect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                   m_tabSettingsWidget, &TabSettingsWidget::setTabSettings);
        disconnect(m_preferences, &ICodeStylePreferences::currentPreferencesChanged,
                   this, &SimpleCodeStylePreferencesWidget::slotCurrentPreferencesChanged);
        disconnect(m_tabSettingsWidget, &TabSettingsWidget::settingsChanged,
                   this, &SimpleCodeStylePreferencesWidget::slotTabSettingsChanged);
    }
    m_preferences = preferences;
    // fillup new
    if (m_preferences) {
        slotCurrentPreferencesChanged(m_preferences->currentPreferences());
        m_tabSettingsWidget->setTabSettings(m_preferences->currentTabSettings());

        connect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                m_tabSettingsWidget, &TabSettingsWidget::setTabSettings);
        connect(m_preferences, &ICodeStylePreferences::currentPreferencesChanged,
                this, &SimpleCodeStylePreferencesWidget::slotCurrentPreferencesChanged);
        connect(m_tabSettingsWidget, &TabSettingsWidget::settingsChanged,
                this, &SimpleCodeStylePreferencesWidget::slotTabSettingsChanged);
    }
    m_tabSettingsWidget->setEnabled(m_preferences);
}

void SimpleCodeStylePreferencesWidget::slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences)
{
    m_tabSettingsWidget->setEnabled(!preferences->isReadOnly() && !m_preferences->currentDelegate());
}

void SimpleCodeStylePreferencesWidget::slotTabSettingsChanged(const TextEditor::TabSettings &settings)
{
    if (!m_preferences)
        return;

    ICodeStylePreferences *current = m_preferences->currentPreferences();
    if (!current)
        return;

    current->setTabSettings(settings);
}

TabSettingsWidget *SimpleCodeStylePreferencesWidget::tabSettingsWidget() const
{
    return m_tabSettingsWidget;
}

} // namespace TextEditor
