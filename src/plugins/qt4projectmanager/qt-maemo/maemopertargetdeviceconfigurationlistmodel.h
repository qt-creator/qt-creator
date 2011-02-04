/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
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
**************************************************************************/
#ifndef MAEMOPERTARGETDEVICECONFIGURATIONLISTMODEL_H
#define MAEMOPERTARGETDEVICECONFIGURATIONLISTMODEL_H

#include "maemodeviceconfigurations.h"
#include "maemoglobal.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QSharedPointer>

namespace ProjectExplorer { class Target; }

namespace Qt4ProjectManager {
namespace Internal {

class MaemoPerTargetDeviceConfigurationListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit MaemoPerTargetDeviceConfigurationListModel(ProjectExplorer::Target *target);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
        int role = Qt::DisplayRole) const;

    QSharedPointer<const MaemoDeviceConfig> deviceAt(int idx) const;
    QSharedPointer<const MaemoDeviceConfig> defaultDeviceConfig() const;
    QSharedPointer<const MaemoDeviceConfig> find(MaemoDeviceConfig::Id id) const;
    int indexForInternalId(MaemoDeviceConfig::Id id) const;

signals:
    void updated();

private:
    MaemoGlobal::MaemoVersion m_targetOsVersion;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOPERTARGETDEVICECONFIGURATIONLISTMODEL_H
