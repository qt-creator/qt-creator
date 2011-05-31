/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef MAEMOREMOTEPROCESSLIST_H
#define MAEMOREMOTEPROCESSLIST_H

#include <QtCore/QAbstractTableModel>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <utils/ssh/sshremoteprocessrunner.h>

namespace RemoteLinux {
class LinuxDeviceConfiguration;

namespace Internal {

class MaemoRemoteProcessList : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit MaemoRemoteProcessList(const QSharedPointer<const LinuxDeviceConfiguration> &devConfig,
        QObject *parent = 0);
    ~MaemoRemoteProcessList();
    void update();
    void killProcess(int row);

signals:
    void error(const QString &errorMsg);
    void processKilled();

private slots:
    void handleRemoteStdOut(const QByteArray &output);
    void handleRemoteStdErr(const QByteArray &output);
    void handleConnectionError();
    void handleRemoteProcessFinished(int exitStatus);

private:
    enum State { Inactive, Listing, Killing };

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
        int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;

    void buildProcessList();
    void stop();
    void startProcess(const QByteArray &cmdLine, State newState);

    const QSharedPointer<Utils::SshRemoteProcessRunner> m_process;
    QByteArray m_remoteStdout;
    QByteArray m_remoteStderr;
    QString m_errorMsg;
    State m_state;

    struct RemoteProc {
        RemoteProc(int pid, const QString &cmdLine)
            : pid(pid), cmdLine(cmdLine) {}
        int pid;
        QString cmdLine;
    };
    QList<RemoteProc> m_remoteProcs;
    const QSharedPointer<const LinuxDeviceConfiguration> m_devConfig;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMOREMOTEPROCESSLIST_H
