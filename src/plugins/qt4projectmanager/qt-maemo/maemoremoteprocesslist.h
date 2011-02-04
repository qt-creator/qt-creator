/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
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

#ifndef MAEMOREMOTEPROCESSLIST_H
#define MAEMOREMOTEPROCESSLIST_H

#include <QtCore/QAbstractTableModel>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <coreplugin/ssh/sshremoteprocessrunner.h>

namespace Qt4ProjectManager {
namespace Internal {
class MaemoDeviceConfig;

class MaemoRemoteProcessList : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit MaemoRemoteProcessList(const QSharedPointer<const MaemoDeviceConfig> &devConfig,
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

    const QSharedPointer<Core::SshRemoteProcessRunner> m_process;
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
    const QSharedPointer<const MaemoDeviceConfig> m_devConfig;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOREMOTEPROCESSLIST_H
