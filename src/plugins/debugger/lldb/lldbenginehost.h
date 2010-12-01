/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_LLDBENGINE_HOST_H
#define DEBUGGER_LLDBENGINE_HOST_H

#include "ipcenginehost.h"
#include <coreplugin/ssh/ssherrors.h>
#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>
#include <coreplugin/ssh/sshremoteprocessrunner.h>

#include <QtCore/QProcess>
#include <QtCore/QQueue>

namespace Debugger {
namespace Internal {

class SshIODevice : public QIODevice
{
Q_OBJECT
public:
    SshIODevice(Core::SshRemoteProcessRunner::Ptr r);
    virtual qint64 bytesAvailable () const;
    virtual qint64 writeData (const char * data, qint64 maxSize);
    virtual qint64 readData (char * data, qint64 maxSize);
private slots:
    void processStarted();
    void outputAvailable(const QByteArray &output);
    void errorOutputAvailable(const QByteArray &output);
private:
    Core::SshRemoteProcessRunner::Ptr runner;
    Core::SshRemoteProcess::Ptr proc;
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
    Core::SshRemoteProcessRunner::Ptr m_ssh;
protected:
    void nuke();
private slots:
    void sshConnectionError(Core::SshError);
    void finished(int, QProcess::ExitStatus);
    void stderrReady();
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_LLDBENGINE_HOST_H
