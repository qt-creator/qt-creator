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
#ifndef REMOTELINUXPROCESSLIST_H
#define REMOTELINUXPROCESSLIST_H

#include "remotelinux_export.h"

#include <QAbstractTableModel>
#include <QList>
#include <QSharedPointer>

namespace RemoteLinux {
class LinuxDeviceConfiguration;

namespace Internal {
class AbstractRemoteLinuxProcessListPrivate;
}

class REMOTELINUX_EXPORT RemoteProcess
{
public:
    RemoteProcess() : pid(0) {}
    bool operator<(const RemoteProcess &other) const;

    int pid;
    QString cmdLine;
    QString exe;
};

class REMOTELINUX_EXPORT AbstractRemoteLinuxProcessList : public QAbstractTableModel
{
    Q_OBJECT
    friend class Internal::AbstractRemoteLinuxProcessListPrivate;
public:
    ~AbstractRemoteLinuxProcessList();

    void update();
    void killProcess(int row);
    RemoteProcess at(int row) const;

signals:
    void processListUpdated();
    void error(const QString &errorMsg);
    void processKilled();

protected:
    AbstractRemoteLinuxProcessList(const QSharedPointer<const LinuxDeviceConfiguration> &devConfig,
        QObject *parent = 0);

    QSharedPointer<const LinuxDeviceConfiguration> deviceConfiguration() const;

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
    virtual QString killProcessCommandLine(const RemoteProcess &process) const = 0;
    virtual QList<RemoteProcess> buildProcessList(const QString &listProcessesReply) const = 0;

    void startProcess(const QString &cmdLine);
    void setFinished();

    Internal::AbstractRemoteLinuxProcessListPrivate * const d;
};


class REMOTELINUX_EXPORT GenericRemoteLinuxProcessList : public AbstractRemoteLinuxProcessList
{
    Q_OBJECT
public:
    GenericRemoteLinuxProcessList(const QSharedPointer<const LinuxDeviceConfiguration> &devConfig,
        QObject *parent = 0);

protected:
    QString listProcessesCommandLine() const;
    QString killProcessCommandLine(const RemoteProcess &process) const;
    QList<RemoteProcess> buildProcessList(const QString &listProcessesReply) const;
};

} // namespace RemoteLinux

#endif // REMOTELINUXPROCESSLIST_H
