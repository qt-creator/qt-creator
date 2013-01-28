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
    void outputAvailable();
    void errorOutputAvailable();
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
