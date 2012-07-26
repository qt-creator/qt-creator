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

#ifndef DEVICEPROCESSLIST_H
#define DEVICEPROCESSLIST_H

#include "idevice.h"

#include <QAbstractTableModel>
#include <QList>
#include <QSharedPointer>

namespace ProjectExplorer {

namespace Internal {
class DeviceProcessListPrivate;
}

class PROJECTEXPLORER_EXPORT DeviceProcess
{
public:
    DeviceProcess() : pid(0) {}
    bool operator<(const DeviceProcess &other) const;

    int pid;
    QString cmdLine;
    QString exe;
};

class PROJECTEXPLORER_EXPORT DeviceProcessList : public QAbstractTableModel
{
    Q_OBJECT
    friend class Internal::DeviceProcessListPrivate;
public:
    ~DeviceProcessList();

    void update();
    void killProcess(int row);
    DeviceProcess at(int row) const;

signals:
    void processListUpdated();
    void error(const QString &errorMsg);
    void processKilled();

protected:
    DeviceProcessList(const IDevice::ConstPtr &devConfig, QObject *parent = 0);

private slots:
    void handleConnectionError();
    void handleRemoteProcessFinished(int exitStatus);

private:
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    virtual QString listProcessesCommandLine() const = 0;
    virtual QString killProcessCommandLine(const DeviceProcess &process) const = 0;
    virtual QList<DeviceProcess> buildProcessList(const QString &listProcessesReply) const = 0;

    void startProcess(const QString &cmdLine);
    void setFinished();

    Internal::DeviceProcessListPrivate * const d;
};


class PROJECTEXPLORER_EXPORT GenericLinuxProcessList : public DeviceProcessList
{
    Q_OBJECT

public:
    GenericLinuxProcessList(const IDevice::ConstPtr &devConfig,
        QObject *parent = 0);

protected:
    QString listProcessesCommandLine() const;
    QString killProcessCommandLine(const DeviceProcess &process) const;
    QList<DeviceProcess> buildProcessList(const QString &listProcessesReply) const;
};

} // namespace ProjectExplorer

#endif // DEVICEPROCESSLIST_H
