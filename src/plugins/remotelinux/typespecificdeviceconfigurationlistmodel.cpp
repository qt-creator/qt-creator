/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "typespecificdeviceconfigurationlistmodel.h"

#include "linuxdeviceconfigurations.h"
#include "remotelinux_constants.h"

namespace RemoteLinux {
namespace Internal {

TypeSpecificDeviceConfigurationListModel::TypeSpecificDeviceConfigurationListModel(const QString &osType,
    QObject *parent) : QAbstractListModel(parent), m_targetOsType(osType)
{
    const LinuxDeviceConfigurations * const devConfs
        = LinuxDeviceConfigurations::instance();
    connect(devConfs, SIGNAL(modelReset()), this, SIGNAL(modelReset()));
    connect(devConfs, SIGNAL(updated()), this, SIGNAL(updated()));
}

TypeSpecificDeviceConfigurationListModel::~TypeSpecificDeviceConfigurationListModel()
{
}

int TypeSpecificDeviceConfigurationListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    int count = 0;
    const LinuxDeviceConfigurations * const devConfs
        = LinuxDeviceConfigurations::instance();
    const int devConfsCount = devConfs->rowCount();
    if (m_targetOsType == QLatin1String(Constants::GenericLinuxOsType))
        return devConfsCount;
    for (int i = 0; i < devConfsCount; ++i) {
        if (devConfs->deviceAt(i)->osType() == m_targetOsType)
            ++count;
    }
    return count;
}

QVariant TypeSpecificDeviceConfigurationListModel::data(const QModelIndex &index,
    int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || role != Qt::DisplayRole)
        return QVariant();
    const LinuxDeviceConfiguration::ConstPtr &devConf = deviceAt(index.row());
    Q_ASSERT(devConf);
    QString displayedName = devConf->displayName();
    if (devConf->isDefault() && devConf->osType() == m_targetOsType)
        displayedName += QLatin1Char(' ') + tr("(default)");
    return displayedName;
}

LinuxDeviceConfiguration::ConstPtr TypeSpecificDeviceConfigurationListModel::deviceAt(int idx) const
{
    int currentRow = -1;
    const LinuxDeviceConfigurations * const devConfs
        = LinuxDeviceConfigurations::instance();
    if (m_targetOsType == QLatin1String(Constants::GenericLinuxOsType))
        return devConfs->deviceAt(idx);
    const int devConfsCount = devConfs->rowCount();
    for (int i = 0; i < devConfsCount; ++i) {
        if (devConfs->deviceAt(i)->osType() == m_targetOsType) {
            if (++currentRow == idx)
                return devConfs->deviceAt(i);
        }
    }
    Q_ASSERT(false);
    return LinuxDeviceConfiguration::ConstPtr();
}

LinuxDeviceConfiguration::ConstPtr TypeSpecificDeviceConfigurationListModel::defaultDeviceConfig() const
{
    return LinuxDeviceConfigurations::instance()->defaultDeviceConfig(m_targetOsType);
}

LinuxDeviceConfiguration::ConstPtr TypeSpecificDeviceConfigurationListModel::find(LinuxDeviceConfiguration::Id id) const
{
    const LinuxDeviceConfiguration::ConstPtr &devConf
        = LinuxDeviceConfigurations::instance()->find(id);
    return devConf && (devConf->osType() == m_targetOsType
            || m_targetOsType == QLatin1String(Constants::GenericLinuxOsType))
        ? devConf : defaultDeviceConfig();
}

int TypeSpecificDeviceConfigurationListModel::indexForInternalId(LinuxDeviceConfiguration::Id id) const
{
    const int count = rowCount();
    for (int i = 0; i < count; ++i) {
        if (deviceAt(i)->internalId() == id)
            return i;
    }
    return -1;
}

} // namespace Internal
} // namespace RemoteLinux
