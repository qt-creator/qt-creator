/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEBUGGER_LLDBENGINE_HOST_H
#define DEBUGGER_LLDBENGINE_HOST_H

#include "ipcenginehost.h"
#include <ssh/ssherrors.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>
#include <ssh/sshremoteprocessrunner.h>

#include <QProcess>
#include <QQueue>

namespace Debugger {
namespace Internal {

class SshIODevice : public QIODevice
{
Q_OBJECT
public:
    SshIODevice(QSsh::SshRemoteProcessRunner *r);
    ~SshIODevice();
    virtual qint64 bytesAvailable () const;
    virtual qint64 writeData (const char * data, qint64 maxSize);
    virtual qint64 readData (char * data, qint64 maxSize);
private slots:
    void processStarted();
    void outputAvailable(const QByteArray &output);
    void errorOutputAvailable(const QByteArray &output);
private:
    QSsh::SshRemoteProcessRunner *runner;
    QSsh::SshRemoteProcess::Ptr proc;
    int buckethead;
    QQueue<QByteArray> buckets;
    QByteArray startupbuffer;
};

class LldbEngineHost : public IPCEngineHost
{
    Q_OBJECT

public:
    explicit LldbEngineHost(const DebuggerStartParameters &startParameters);
    ~LldbEngineHost();

private:
    QProcess *m_guestProcess;
    QSsh::SshRemoteProcessRunner *m_ssh;
protected:
    void nuke();
private slots:
    void sshConnectionError(QSsh::SshError);
    void finished(int, QProcess::ExitStatus);
    void stderrReady();
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_LLDBENGINE_HOST_H
