/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/
#include "devicesettingspage.h"

#include "devicesettingswidget.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <QCoreApplication>
#include <QString>
#include <QIcon>

namespace ProjectExplorer {
namespace Internal {

DeviceSettingsPage::DeviceSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
    setId(QLatin1String(Constants::DEVICE_SETTINGS_PAGE_ID));
    setDisplayName(tr("Devices"));
    setCategory(QLatin1String(Constants::DEVICE_SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer", "Devices"));
    setCategoryIcon(QLatin1String(":/projectexplorer/images/MaemoDevice.png"));
}

bool DeviceSettingsPage::matches(const QString &searchKeyWord) const
{
    return m_keywords.contains(searchKeyWord, Qt::CaseInsensitive);
}

QWidget *DeviceSettingsPage::createPage(QWidget *parent)
{
    m_widget = new DeviceSettingsWidget(parent);
    if (m_keywords.isEmpty())
        m_keywords = m_widget->searchKeywords();
    return m_widget;
}

void DeviceSettingsPage::apply()
{
    m_widget->saveSettings();
}

void DeviceSettingsPage::finish()
{
}

} // namespace Internal
} // namespace ProjectExplorer
