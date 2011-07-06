/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "tabpreferenceswidget.h"
#include "ui_tabpreferenceswidget.h"
#include "tabpreferences.h"

#include <QtCore/QTextStream>

namespace TextEditor {

TabPreferencesWidget::TabPreferencesWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::TabPreferencesWidget),
    m_tabPreferences(0)
{
    m_ui->setupUi(this);
    m_ui->fallbackWidget->setLabelText(tr("Tab settings:"));
    m_ui->tabSettingsWidget->setEnabled(false);
}

TabPreferencesWidget::~TabPreferencesWidget()
{
    delete m_ui;
}

void TabPreferencesWidget::setTabPreferences(TabPreferences *tabPreferences)
{
    if (m_tabPreferences == tabPreferences)
        return; // nothing changes

    // cleanup old
    if (m_tabPreferences) {
        disconnect(m_tabPreferences, SIGNAL(currentSettingsChanged(TextEditor::TabSettings)),
                m_ui->tabSettingsWidget, SLOT(setSettings(TextEditor::TabSettings)));
        disconnect(m_tabPreferences, SIGNAL(currentPreferencesChanged(TextEditor::IFallbackPreferences*)),
                this, SLOT(slotCurrentPreferencesChanged(TextEditor::IFallbackPreferences*)));
        disconnect(m_ui->tabSettingsWidget, SIGNAL(settingsChanged(TextEditor::TabSettings)),
                this, SLOT(slotTabSettingsChanged(TextEditor::TabSettings)));
    }
    m_tabPreferences = tabPreferences;
    m_ui->fallbackWidget->setFallbackPreferences(tabPreferences);
    // fillup new
    if (m_tabPreferences) {
        slotCurrentPreferencesChanged(m_tabPreferences->currentPreferences());
        m_ui->tabSettingsWidget->setSettings(m_tabPreferences->currentSettings());

        connect(m_tabPreferences, SIGNAL(currentSettingsChanged(TextEditor::TabSettings)),
                m_ui->tabSettingsWidget, SLOT(setSettings(TextEditor::TabSettings)));
        connect(m_tabPreferences, SIGNAL(currentPreferencesChanged(TextEditor::IFallbackPreferences*)),
                this, SLOT(slotCurrentPreferencesChanged(TextEditor::IFallbackPreferences*)));
        connect(m_ui->tabSettingsWidget, SIGNAL(settingsChanged(TextEditor::TabSettings)),
                this, SLOT(slotTabSettingsChanged(TextEditor::TabSettings)));
    } else {
        m_ui->tabSettingsWidget->setEnabled(false);
    }
}

void TabPreferencesWidget::slotCurrentPreferencesChanged(TextEditor::IFallbackPreferences *preferences)
{
    m_ui->tabSettingsWidget->setEnabled(!preferences->isReadOnly() && m_tabPreferences->isFallbackEnabled(m_tabPreferences->currentFallback()));
}

void TabPreferencesWidget::slotTabSettingsChanged(const TextEditor::TabSettings &settings)
{
    if (!m_tabPreferences)
        return;

    TabPreferences *current = qobject_cast<TabPreferences *>(m_tabPreferences->currentPreferences());
    if (!current)
        return;

    current->setSettings(settings);
}

QString TabPreferencesWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream(&rc)
            << sep << m_ui->fallbackWidget->searchKeywords()
            << sep << m_ui->tabSettingsWidget->searchKeywords()
               ;
    rc.remove(QLatin1Char('&'));
    return rc;
}

void TabPreferencesWidget::setFallbacksVisible(bool on)
{
    m_ui->fallbackWidget->setFallbacksVisible(on);
}

void TabPreferencesWidget::setFlat(bool on)
{
     m_ui->tabSettingsWidget->setFlat(on);
}

void TabPreferencesWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

} // namespace TextEditor
