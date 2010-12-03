/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Assistant of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemosettingspages.h"

#include "maemoconstants.h"
#include "maemodeviceconfigurationssettingswidget.h"
#include "maemoqemusettingswidget.h"

#include <QtCore/QCoreApplication>

namespace Qt4ProjectManager {
namespace Internal {

MaemoDeviceConfigurationsSettingsPage::MaemoDeviceConfigurationsSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
}

MaemoDeviceConfigurationsSettingsPage::~MaemoDeviceConfigurationsSettingsPage()
{
}

QString MaemoDeviceConfigurationsSettingsPage::id() const
{
    return QLatin1String("ZZ.Maemo Device Configurations");
}

QString MaemoDeviceConfigurationsSettingsPage::displayName() const
{
    return tr("Maemo Device Configurations");
}

QString MaemoDeviceConfigurationsSettingsPage::category() const
{
    return QLatin1String(Constants::MAEMO_SETTINGS_CATEGORY);
}

QString MaemoDeviceConfigurationsSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("Qt4ProjectManager",
        Constants::MAEMO_SETTINGS_TR_CATEGORY);
}

QIcon MaemoDeviceConfigurationsSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Constants::MAEMO_SETTINGS_CATEGORY_ICON));
}

bool MaemoDeviceConfigurationsSettingsPage::matches(const QString &searchKeyWord) const
{
    return m_keywords.contains(searchKeyWord, Qt::CaseInsensitive);
}

QWidget *MaemoDeviceConfigurationsSettingsPage::createPage(QWidget *parent)
{
    m_widget = new MaemoDeviceConfigurationsSettingsWidget(parent);
    if (m_keywords.isEmpty())
        m_keywords = m_widget->searchKeywords();
    return m_widget;
}

void MaemoDeviceConfigurationsSettingsPage::apply()
{
    m_widget->saveSettings();
}

void MaemoDeviceConfigurationsSettingsPage::finish()
{
}


MaemoQemuSettingsPage::MaemoQemuSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
}

MaemoQemuSettingsPage::~MaemoQemuSettingsPage()
{
}

QString MaemoQemuSettingsPage::id() const
{
    return QLatin1String("ZZ.Qemu Settings");
}

QString MaemoQemuSettingsPage::displayName() const
{
    return tr("Qemu Settings");
}

QString MaemoQemuSettingsPage::category() const
{
    return QLatin1String(Constants::MAEMO_SETTINGS_CATEGORY);
}

QString MaemoQemuSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("Qt4ProjectManager",
        Constants::MAEMO_SETTINGS_TR_CATEGORY);
}

QIcon MaemoQemuSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Constants::MAEMO_SETTINGS_CATEGORY_ICON));
}

bool MaemoQemuSettingsPage::matches(const QString &searchKeyWord) const
{
    return m_widget->keywords().contains(searchKeyWord, Qt::CaseInsensitive);
}

QWidget *MaemoQemuSettingsPage::createPage(QWidget *parent)
{
    m_widget = new MaemoQemuSettingsWidget(parent);
    return m_widget;
}

void MaemoQemuSettingsPage::apply()
{
    m_widget->saveSettings();
}

void MaemoQemuSettingsPage::finish()
{
}

} // namespace Internal
} // namespace Qt4ProjectManager
