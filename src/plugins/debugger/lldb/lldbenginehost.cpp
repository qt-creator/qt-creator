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

#include "lldbenginehost.h"

#include "debuggerstartparameters.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggerdialogs.h"
#include "debuggerplugin.h"
#include "debuggerstringutils.h"

#include "breakhandler.h"
#include "breakpoint.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "threadshandler.h"
#include "disassembleragent.h"
#include "memoryagent.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QProcess>
#include <QFileInfo>
#include <QThread>
#include <QCoreApplication>

namespace Debugger {
namespace Internal {

SshIODevice::SshIODevice(QSsh::SshRemoteProcessRunner *r)
    : runner(r)
    , buckethead(0)
{
    setOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered);
    connect (runner, SIGNAL(processStarted()), this, SLOT(processStarted()));
    connect(runner, SIGNAL(readyReadStandardOutput()), this, SLOT(outputAvailable()));
    connect(runner, SIGNAL(readyReadStandardError()), this, SLOT(errorOutputAvailable()));
}

SshIODevice::~SshIODevice()
{
    delete runner;
}

qint64 SshIODevice::bytesAvailable () const
{
    qint64 r = QIODevice::bytesAvailable();
    foreach (const QByteArray &bucket, buckets)
        r += bucket.size();
    r-= buckethead;
    return r;
}
qint64 SshIODevice::writeData (const char * data, qint64 maxSize)
{
    if (proc == 0) {
        startupbuffer += QByteArray::fromRawData(data, maxSize);
        return maxSize;
    }
    proc->write(data, maxSize);
    return maxSize;
}
qint64 SshIODevice::readData (char * data, qint64 maxSize)
{
    if (proc == 0)
        return 0;
    qint64 size = maxSize;
    while (size > 0) {
        if (!buckets.size())
            return maxSize - size;
        QByteArray &bucket = buckets.head();
        if ((size + buckethead) >= bucket.size()) {
            int d =  bucket.size() - buckethead;
            memcpy(data, bucket.data() + buckethead, d);
            data += d;
            size -= d;
            buckets.dequeue();
            buckethead = 0;
        } else {
            memcpy(data, bucket.data() + buckethead, size);
            data += size;
            buckethead += size;
            size = 0;
        }
    }
    return maxSize - size;
}

void SshIODevice::processStarted()
{
    runner->writeDataToProcess(startupbuffer);
}

void SshIODevice::outputAvailable()
{
    buckets.enqueue(runner->readAllStandardOutput());
    emit readyRead();
}

void SshIODevice::errorOutputAvailable()
{
    fprintf(stderr, "%s", runner->readAllStandardError().data());
}


LldbEngineHost::LldbEngineHost(const DebuggerStartParameters &startParameters)
    :IPCEngineHost(startParameters), m_ssh(0)
{
    showMessage(QLatin1String("setting up coms"));
    setObjectName(QLatin1String("LLDBEngine"));

    if (startParameters.startMode == StartRemoteEngine)
    {
        m_guestProcess = 0;
        QSsh::SshRemoteProcessRunner * const runner = new QSsh::SshRemoteProcessRunner;
        connect (runner, SIGNAL(connectionError(QSsh::SshError)),
                this, SLOT(sshConnectionError(QSsh::SshError)));
        runner->run(startParameters.serverStartScript.toUtf8(), startParameters.connParams);
        setGuestDevice(new SshIODevice(runner));
    } else  {
        m_guestProcess = new QProcess(this);

        connect(m_guestProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
                this, SLOT(finished(int,QProcess::ExitStatus)));

        connect(m_guestProcess, SIGNAL(readyReadStandardError()), this,
                SLOT(stderrReady()));


        QString a = Core::ICore::resourcePath() + QLatin1String("/qtcreator-lldb");
        if (getenv("QTC_LLDB_GUEST") != 0)
            a = QString::fromLocal8Bit(getenv("QTC_LLDB_GUEST"));

        showStatusMessage(QString(QLatin1String("starting %1")).arg(a));

        m_guestProcess->start(a, QStringList(), QIODevice::ReadWrite | QIODevice::Unbuffered);
        m_guestProcess->setReadChannel(QProcess::StandardOutput);

        if (!m_guestProcess->waitForStarted()) {
            showStatusMessage(tr("qtcreator-lldb failed to start: %1").arg(m_guestProcess->errorString()));
            notifyEngineSpontaneousShutdown();
            return;
        }

        setGuestDevice(m_guestProcess);
    }
}

LldbEngineHost::~LldbEngineHost()
{
    showMessage(QLatin1String("tear down qtcreator-lldb"));

    if (m_guestProcess) {
        disconnect(m_guestProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
                this, SLOT(finished(int,QProcess::ExitStatus)));


        m_guestProcess->terminate();
        m_guestProcess->kill();
    }
    if (m_ssh && m_ssh->isProcessRunning()) {
        // TODO: openssh doesn't do that

        m_ssh->sendSignalToProcess(QSsh::SshRemoteProcess::KillSignal);
    }
}

void LldbEngineHost::nuke()
{
    stderrReady();
    showMessage(QLatin1String("Nuke engaged. Bug in Engine/IPC or incompatible IPC versions. "), LogError);
    showStatusMessage(tr("Fatal engine shutdown. Consult debugger log for details."));
    m_guestProcess->terminate();
    m_guestProcess->kill();
    notifyEngineSpontaneousShutdown();
}
void LldbEngineHost::sshConnectionError(QSsh::SshError e)
{
    showStatusMessage(tr("SSH connection error: %1").arg(e));
}

void LldbEngineHost::finished(int, QProcess::ExitStatus status)
{
    showMessage(QString(QLatin1String("guest went bye bye. exit status: %1 and code: %2"))
            .arg(status).arg(m_guestProcess->exitCode()), LogError);
    nuke();
}

void LldbEngineHost::stderrReady()
{
    fprintf(stderr,"%s", m_guestProcess->readAllStandardError().data());
}

DebuggerEngine *createLldbEngine(const DebuggerStartParameters &startParameters)
{
    return new LldbEngineHost(startParameters);
}

} // namespace Internal
} // namespace Debugger

