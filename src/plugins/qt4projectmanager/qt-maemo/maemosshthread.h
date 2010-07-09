/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
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

#ifndef MAEMOSSHTHREAD_H
#define MAEMOSSHTHREAD_H

#include "maemodeviceconfigurations.h"

#include <coreplugin/ssh/sshconnection.h>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>

namespace Qt4ProjectManager {
namespace Internal {

template<class SshConnection> class MaemoSshThread : public QThread
{
    Q_DISABLE_COPY(MaemoSshThread)
public:
    QString error() const { return m_error; }
    bool hasError() const { return !m_error.isEmpty(); }
    void stop();
    virtual void run();
    ~MaemoSshThread();

protected:
    MaemoSshThread(const Core::SshServerInfo &server);
    void createConnection();
    bool stopRequested() const { return m_stopRequested; }
    void waitForStop();

    typename SshConnection::Ptr m_connection;

private:
    virtual bool runInternal() = 0;

    const Core::SshServerInfo m_server;
    bool m_stopRequested;
    QString m_error;
    QMutex m_mutex;
    QWaitCondition m_waitCond;
};


class MaemoSshRunner : public MaemoSshThread<Core::InteractiveSshConnection>
{
    Q_OBJECT
    Q_DISABLE_COPY(MaemoSshRunner)
public:
    MaemoSshRunner(const Core::SshServerInfo &server, const QString &command);

signals:
    void remoteOutput(const QString &output);

private:
    virtual bool runInternal();
    Q_SLOT void handleRemoteOutput(const QByteArray &output);
    void initState();
    QByteArray filterTerminalControlChars(const QByteArray &data);

    static const QByteArray EndMarker;

    const QString m_command;
    const char *m_prompt;
    int m_endMarkerCount;
    bool m_promptEncountered;
    QByteArray m_potentialEndMarkerPrefix;
};


class MaemoSshDeployer : public MaemoSshThread<Core::SftpConnection>
{
    Q_OBJECT
    Q_DISABLE_COPY(MaemoSshDeployer)
public:
    MaemoSshDeployer(const Core::SshServerInfo &server,
                     const QList<Core::SftpTransferInfo> &deploySpecs);

signals:
    void fileCopied(const QString &filePath);

private:
    virtual bool runInternal();

    const QList<Core::SftpTransferInfo> m_deploySpecs;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOSSHTHREAD_H
