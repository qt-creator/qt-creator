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

#ifndef DEVICEPROCESSLIST_H
#define DEVICEPROCESSLIST_H

#include "idevice.h"

#include <QAbstractItemModel>
#include <QList>

namespace ProjectExplorer {

namespace Internal { class DeviceProcessListPrivate; }

class PROJECTEXPLORER_EXPORT DeviceProcess
{
public:
    DeviceProcess() : pid(0) {}
    bool operator<(const DeviceProcess &other) const;

    int pid;
    QString cmdLine;
    QString exe;
};

class PROJECTEXPLORER_EXPORT DeviceProcessList : public QAbstractItemModel
{
    Q_OBJECT

public:
    DeviceProcessList(const IDevice::ConstPtr &device, QObject *parent = 0);
    ~DeviceProcessList();

    void update();
    void killProcess(int row);
    DeviceProcess at(int row) const;

    static QList<DeviceProcess> localProcesses();

signals:
    void processListUpdated();
    void error(const QString &errorMsg);
    void processKilled();

protected:
    void reportError(const QString &message);
    void reportProcessKilled();
    void reportProcessListUpdated(const QList<DeviceProcess> &processes);

    IDevice::ConstPtr device() const;

private:
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex parent(const QModelIndex &) const;
    bool hasChildren(const QModelIndex &parent) const;

    virtual void doUpdate() = 0;
    virtual void doKillProcess(const DeviceProcess &process) = 0;

    void setFinished();

    Internal::DeviceProcessListPrivate * const d;
};

} // namespace ProjectExplorer

#endif // DEVICEPROCESSLIST_H
