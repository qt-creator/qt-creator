/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "simplecodestylepreferenceswidget.h"
#include "icodestylepreferences.h"
#include "tabsettings.h"
#include "tabsettingswidget.h"

#include <QVBoxLayout>

namespace TextEditor {

SimpleCodeStylePreferencesWidget::SimpleCodeStylePreferencesWidget(QWidget *parent) :
    QWidget(parent),
    m_preferences(0)
{
    m_tabSettingsWidget = new TabSettingsWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
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

void SimpleCodeStylePreferencesWidget::setFlat(bool on)
{
    m_tabSettingsWidget->setFlat(on);
}

TabSettingsWidget *SimpleCodeStylePreferencesWidget::tabSettingsWidget() const
{
    return m_tabSettingsWidget;
}

} // namespace TextEditor
