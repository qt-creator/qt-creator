/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
        disconnect(m_preferences, SIGNAL(currentSettingsChanged(TextEditor::TabSettings)),
                m_tabSettingsWidget, SLOT(setSettings(TextEditor::TabSettings)));
        disconnect(m_preferences, SIGNAL(currentPreferencesChanged(TextEditor::ICodeStylePreferences*)),
                this, SLOT(slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences*)));
        disconnect(m_tabSettingsWidget, SIGNAL(settingsChanged(TextEditor::TabSettings)),
                this, SLOT(slotTabSettingsChanged(TextEditor::TabSettings)));
    }
    m_preferences = preferences;
    // fillup new
    if (m_preferences) {
        slotCurrentPreferencesChanged(m_preferences->currentPreferences());
        m_tabSettingsWidget->setTabSettings(m_preferences->currentTabSettings());

        connect(m_preferences, SIGNAL(currentTabSettingsChanged(TextEditor::TabSettings)),
                m_tabSettingsWidget, SLOT(setTabSettings(TextEditor::TabSettings)));
        connect(m_preferences, SIGNAL(currentPreferencesChanged(TextEditor::ICodeStylePreferences*)),
                this, SLOT(slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences*)));
        connect(m_tabSettingsWidget, SIGNAL(settingsChanged(TextEditor::TabSettings)),
                this, SLOT(slotTabSettingsChanged(TextEditor::TabSettings)));
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

QString SimpleCodeStylePreferencesWidget::searchKeywords() const
{
    return m_tabSettingsWidget->searchKeywords();
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
