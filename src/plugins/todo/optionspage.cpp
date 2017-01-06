/****************************************************************************
**
** Copyright (C) 2016 Dmitry Savchenko
** Copyright (C) 2016 Vasiliy Sorokin
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

#include "optionspage.h"
#include "constants.h"
#include "optionsdialog.h"

#include <coreplugin/icore.h>

#include <QList>
#include <QIcon>
#include <QMessageBox>

Todo::Internal::OptionsDialog *some = 0;

namespace Todo {
namespace Internal {

OptionsPage::OptionsPage(const Settings &settings, QObject *parent) :
    IOptionsPage(parent),
    m_widget(0)
{
    setSettings(settings);

    setId("TodoSettings");
    setDisplayName(tr("To-Do"));
    setCategory("To-Do");
    setDisplayCategory(tr("To-Do"));
    setCategoryIcon(Utils::Icon(Constants::ICON_TODO));
}

void OptionsPage::setSettings(const Settings &settings)
{
    m_settings = settings;
}

QWidget *OptionsPage::widget()
{
    if (!m_widget) {
        m_widget = new OptionsDialog;
        m_widget->setSettings(m_settings);
    }
    return m_widget;
}

void OptionsPage::apply()
{
    Settings newSettings = m_widget->settings();

    // "apply" itself is interpreted as "use these keywords, also for other themes".
    newSettings.keywordsEdited = true;

    if (newSettings != m_settings) {
        m_settings = newSettings;
        emit settingsChanged(m_settings);
    }
}

void OptionsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace Todo

