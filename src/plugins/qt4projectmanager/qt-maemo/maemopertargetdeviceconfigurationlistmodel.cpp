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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "maemopertargetdeviceconfigurationlistmodel.h"

#include "qt4maemotarget.h"

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

MaemoPerTargetDeviceConfigurationListModel::MaemoPerTargetDeviceConfigurationListModel(Target *target)
    : QAbstractListModel(target)
{
    if (qobject_cast<Qt4Maemo5Target *>(target))
        m_targetOsVersion = MaemoGlobal::Maemo5;
    else if (qobject_cast<Qt4HarmattanTarget *>(target))
        m_targetOsVersion = MaemoGlobal::Maemo6;
    else if (qobject_cast<Qt4MeegoTarget *>(target))
        m_targetOsVersion = MaemoGlobal::Meego;
    else
        Q_ASSERT(false);
    const MaemoDeviceConfigurations * const devConfs
        = MaemoDeviceConfigurations::instance();
    connect(devConfs, SIGNAL(modelReset()), this, SIGNAL(modelReset()));
    connect(devConfs, SIGNAL(updated()), this, SIGNAL(updated()));
}

int MaemoPerTargetDeviceConfigurationListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    int count = 0;
    const MaemoDeviceConfigurations * const devConfs
        = MaemoDeviceConfigurations::instance();
    const int devConfsCount = devConfs->rowCount();
    for (int i = 0; i < devConfsCount; ++i) {
        if (devConfs->deviceAt(i)->osVersion() == m_targetOsVersion)
            ++count;
    }
    return count;
}

QVariant MaemoPerTargetDeviceConfigurationListModel::data(const QModelIndex &index,
    int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || role != Qt::DisplayRole)
        return QVariant();
    const MaemoDeviceConfig::ConstPtr &devConf = deviceAt(index.row());
    Q_ASSERT(devConf);
    QString displayedName = devConf->name();
    if (devConf->isDefault())
        displayedName += QLatin1Char(' ') + tr("(default)");
    return displayedName;
}

MaemoDeviceConfig::ConstPtr MaemoPerTargetDeviceConfigurationListModel::deviceAt(int idx) const
{
    int currentRow = -1;
    const MaemoDeviceConfigurations * const devConfs
        = MaemoDeviceConfigurations::instance();
    const int devConfsCount = devConfs->rowCount();
    for (int i = 0; i < devConfsCount; ++i) {
        if (devConfs->deviceAt(i)->osVersion() == m_targetOsVersion) {
            if (++currentRow == idx)
                return devConfs->deviceAt(i);
        }
    }
    Q_ASSERT(false);
    return MaemoDeviceConfig::ConstPtr();
}

MaemoDeviceConfig::ConstPtr MaemoPerTargetDeviceConfigurationListModel::defaultDeviceConfig() const
{
    return MaemoDeviceConfigurations::instance()->defaultDeviceConfig(m_targetOsVersion);
}

MaemoDeviceConfig::ConstPtr MaemoPerTargetDeviceConfigurationListModel::find(MaemoDeviceConfig::Id id) const
{
    const MaemoDeviceConfig::ConstPtr &devConf
        = MaemoDeviceConfigurations::instance()->find(id);
    return devConf && devConf->osVersion() == m_targetOsVersion
        ? devConf : defaultDeviceConfig();
}

int MaemoPerTargetDeviceConfigurationListModel::indexForInternalId(MaemoDeviceConfig::Id id) const
{
    const int count = rowCount();
    for (int i = 0; i < count; ++i) {
        if (deviceAt(i)->internalId() == id)
            return i;
    }
    return -1;
}

} // namespace Internal
} // namespace Qt4ProjectManager
