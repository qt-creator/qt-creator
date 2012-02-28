/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "remotelinuxsettingspages.h"

#include "linuxdeviceconfigurationssettingswidget.h"
#include "remotelinux_constants.h"

#include <QCoreApplication>
#include <QString>
#include <QIcon>

namespace RemoteLinux {
namespace Internal {

LinuxDeviceConfigurationsSettingsPage::LinuxDeviceConfigurationsSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
}

LinuxDeviceConfigurationsSettingsPage::~LinuxDeviceConfigurationsSettingsPage()
{
}

QString LinuxDeviceConfigurationsSettingsPage::id() const
{
    return pageId();
}

QString LinuxDeviceConfigurationsSettingsPage::displayName() const
{
    return tr("Device Configurations");
}

QString LinuxDeviceConfigurationsSettingsPage::category() const
{
    return pageCategory();
}

QString LinuxDeviceConfigurationsSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("RemoteLinux", "Devices");
}

QIcon LinuxDeviceConfigurationsSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(":/projectexplorer/images/MaemoDevice.png"));
}

bool LinuxDeviceConfigurationsSettingsPage::matches(const QString &searchKeyWord) const
{
    return m_keywords.contains(searchKeyWord, Qt::CaseInsensitive);
}

QWidget *LinuxDeviceConfigurationsSettingsPage::createPage(QWidget *parent)
{
    m_widget = new LinuxDeviceConfigurationsSettingsWidget(parent);
    if (m_keywords.isEmpty())
        m_keywords = m_widget->searchKeywords();
    return m_widget;
}

void LinuxDeviceConfigurationsSettingsPage::apply()
{
    m_widget->saveSettings();
}

void LinuxDeviceConfigurationsSettingsPage::finish()
{
}

QString LinuxDeviceConfigurationsSettingsPage::pageId()
{
    return QLatin1String(Constants::RemoteLinuxSettingsPageId);
}

QString LinuxDeviceConfigurationsSettingsPage::pageCategory()
{
    return QLatin1String(Constants::RemoteLinuxSettingsCategory);
}

} // namespace Internal
} // namespace RemoteLinux
