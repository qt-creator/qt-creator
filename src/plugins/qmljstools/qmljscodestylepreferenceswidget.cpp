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

#include "qmljscodestylepreferenceswidget.h"
#include "qmljscodestylepreferences.h"
#include "qmljscodestylesettingswidget.h"
#include "qmljscodestylesettings.h"

#include <QVBoxLayout>
#include <QLabel>

namespace QmlJSTools {

QmlJSCodeStylePreferencesWidget::QmlJSCodeStylePreferencesWidget(QWidget *parent) :
      QWidget(parent)
{
    m_codeStyleSettingsWidget = new QmlJSCodeStyleSettingsWidget(this);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_codeStyleSettingsWidget);
    layout->setContentsMargins(QMargins());
    m_codeStyleSettingsWidget->setEnabled(false);
}

void QmlJSCodeStylePreferencesWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    if (m_preferences == preferences)
        return; // nothing changes

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
        slotCurrentPreferencesChanged(m_preferences->currentPreferences());

        m_codeStyleSettingsWidget->setCodeStyleSettings(m_preferences->currentCodeStyleSettings());

        connect(m_preferences, &QmlJSCodeStylePreferences::currentCodeStyleSettingsChanged,
                m_codeStyleSettingsWidget, &QmlJSCodeStyleSettingsWidget::setCodeStyleSettings);
        connect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                this, &QmlJSCodeStylePreferencesWidget::slotCurrentPreferencesChanged);
        connect(m_codeStyleSettingsWidget, &QmlJSCodeStyleSettingsWidget::settingsChanged,
                this, &QmlJSCodeStylePreferencesWidget::slotSettingsChanged);
    }
    m_codeStyleSettingsWidget->setEnabled(m_preferences);
}

void QmlJSCodeStylePreferencesWidget::slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences)
{
    m_codeStyleSettingsWidget->setEnabled(!preferences->isReadOnly() && !m_preferences->currentDelegate());
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
