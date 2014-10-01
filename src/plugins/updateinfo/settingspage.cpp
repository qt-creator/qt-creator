/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "settingspage.h"
#include "updateinfoplugin.h"

#include <coreplugin/coreconstants.h>

using namespace UpdateInfo;
using namespace UpdateInfo::Internal;

SettingsPage::SettingsPage(UpdateInfoPlugin *plugin)
    : m_widget(0)
    , m_plugin(plugin)
{
    setId(Constants::FILTER_OPTIONS_PAGE);
    setCategory(Core::Constants::SETTINGS_CATEGORY_CORE);
    setCategoryIcon(QLatin1String(Core::Constants::SETTINGS_CATEGORY_CORE_ICON));
    setDisplayName(QCoreApplication::translate("Update", UpdateInfo::Constants::FILTER_OPTIONS_PAGE));
    setDisplayCategory(QCoreApplication::translate("Core", Core::Constants::SETTINGS_TR_CATEGORY_CORE));
}

QWidget *SettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new QWidget;
        m_ui.setupUi(m_widget);
        m_ui.m_timeTable->setItemText(m_ui.m_timeTable->currentIndex(), QTime(m_plugin->scheduledUpdateTime())
            .toString(QLatin1String("hh:mm")));
    }
    return m_widget;
}

void SettingsPage::apply()
{
    m_plugin->setScheduledUpdateTime(QTime::fromString(m_ui.m_timeTable->currentText(),
        QLatin1String("hh:mm")));
    m_plugin->saveSettings();
}

void SettingsPage::finish()
{
    delete m_widget;
}
