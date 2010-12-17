/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#define QT_NO_CAST_FROM_ASCII

#include "lldbenginehost.h"

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

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

namespace Debugger {
namespace Internal {

SshIODevice::SshIODevice(Core::SshRemoteProcessRunner::Ptr r)
    : runner(r)
    , buckethead(0)
{
    setOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered);
    connect (runner.data(), SIGNAL(processStarted()),
            this, SLOT(processStarted()));
    connect(runner.data(), SIGNAL(processOutputAvailable(const QByteArray &)),
            this, SLOT(outputAvailable(const QByteArray &)));
    connect(runner.data(), SIGNAL(processErrorOutputAvailable(const QByteArray &)),
            this, SLOT(errorOutputAvailable(const QByteArray &)));
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
    proc->sendInput(QByteArray::fromRawData(data, maxSize));
    return maxSize;
}
qint64 SshIODevice::readData (char * data, qint64 maxSize)
{
    if (proc == 0)
        return 0;
    qint64 size = maxSize;
    while (size > 0) {
        if (!buckets.size()) {
            return maxSize - size;
        }
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
    proc = runner->process();
    proc->sendInput(startupbuffer);
}

void SshIODevice::outputAvailable(const QByteArray &output)
{
    buckets.enqueue(output);
    emit readyRead();
}

void SshIODevice::errorOutputAvailable(const QByteArray &output)
{
    fprintf(stderr, "%s", output.data());
}


LldbEngineHost::LldbEngineHost(const DebuggerStartParameters &startParameters)
    :IPCEngineHost(startParameters)
{
    showMessage(QLatin1String("setting up coms"));

    if (startParameters.startMode == StartRemoteEngine)
    {
        m_guestProcess = 0;
        Core::SshRemoteProcessRunner::Ptr runner =
            Core::SshRemoteProcessRunner::create(startParameters.connParams);
        connect (runner.data(), SIGNAL(connectionError(Core::SshError)),
                this, SLOT(sshConnectionError(Core::SshError)));
        runner->run(startParameters.serverStartScript.toUtf8());
        setGuestDevice(new SshIODevice(runner));
    } else  {
        m_guestProcess = new QProcess(this);

        connect(m_guestProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
                this, SLOT(finished(int, QProcess::ExitStatus)));

        connect(m_guestProcess, SIGNAL(readyReadStandardError()), this,
                SLOT(stderrReady()));


        QString a = Core::ICore::instance()->resourcePath() + QLatin1String("/qtcreator-lldb");
        if(getenv("QTC_LLDB_GUEST") != 0)
            a = QString::fromLocal8Bit(getenv("QTC_LLDB_GUEST"));

        showStatusMessage(QString(QLatin1String("starting %1")).arg(a));

        m_guestProcess->start(a, QStringList(), QIODevice::ReadWrite | QIODevice::Unbuffered);
        m_guestProcess->setReadChannel(QProcess::StandardOutput);

        if (!m_guestProcess->waitForStarted()) {
            showStatusMessage(tr("qtcreator-lldb failed to start %1").arg(m_guestProcess->error()));
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
        disconnect(m_guestProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
                this, SLOT(finished (int, QProcess::ExitStatus)));


        m_guestProcess->terminate();
        m_guestProcess->kill();
    }
    if (m_ssh.data() && m_ssh->process().data()) {
        // TODO: openssh doesn't do that

        m_ssh->process()->kill();
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
void LldbEngineHost::sshConnectionError(Core::SshError e)
{
    showStatusMessage(tr("ssh connection error: %1").arg(e));
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

