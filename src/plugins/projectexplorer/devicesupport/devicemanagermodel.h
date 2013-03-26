/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef DEVICEMANAGERMODEL_H
#define DEVICEMANAGERMODEL_H

#include "../projectexplorer_export.h"
#include "idevice.h"

#include <QAbstractListModel>

namespace ProjectExplorer {
namespace Internal { class DeviceManagerModelPrivate; }
class IDevice;
class DeviceManager;

class PROJECTEXPLORER_EXPORT DeviceManagerModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit DeviceManagerModel(const DeviceManager *deviceManager, QObject *parent = 0);
    ~DeviceManagerModel();

    void setFilter(const QList<Core::Id> filter);
    void setTypeFilter(const Core::Id &type);

    IDevice::ConstPtr device(int pos) const;
    Core::Id deviceId(int pos) const;
    int indexOf(IDevice::ConstPtr dev) const;
    int indexForId(Core::Id id) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    void updateDevice(Core::Id id);

private slots:
    void handleDeviceAdded(Core::Id id);
    void handleDeviceRemoved(Core::Id id);
    void handleDeviceUpdated(Core::Id id);
    void handleDeviceListChanged();

private:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool matchesTypeFilter(const IDevice::ConstPtr &dev) const;

    Internal::DeviceManagerModelPrivate * const d;
};

} // namespace ProjectExplorer

#endif // DEVICEMANAGERMODEL_H
