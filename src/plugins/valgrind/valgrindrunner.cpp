/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "valgrindrunner.h"
#include "valgrindprocess.h"

#include <projectexplorer/runnables.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>

#include <QEventLoop>

using namespace ProjectExplorer;

namespace Valgrind {

class ValgrindRunner::Private
{
public:
    ValgrindProcess *process = 0;
    QProcess::ProcessChannelMode channelMode = QProcess::SeparateChannels;
    bool finished = false;
    QString valgrindExecutable;
    QStringList valgrindArguments;
    StandardRunnable debuggee;
    IDevice::ConstPtr device;
};

ValgrindRunner::ValgrindRunner(QObject *parent)
    : QObject(parent), d(new Private)
{
}

ValgrindRunner::~ValgrindRunner()
{
    if (d->process && d->process->isRunning()) {
        // make sure we don't delete the thread while it's still running
        waitForFinished();
    }
    delete d;
    d = 0;
}

void ValgrindRunner::setValgrindExecutable(const QString &executable)
{
    d->valgrindExecutable = executable;
}

QString ValgrindRunner::valgrindExecutable() const
{
    return d->valgrindExecutable;
}

void ValgrindRunner::setValgrindArguments(const QStringList &toolArguments)
{
    d->valgrindArguments = toolArguments;
}

QStringList ValgrindRunner::valgrindArguments() const
{
    return d->valgrindArguments;
}

QStringList ValgrindRunner::fullValgrindArguments() const
{
    QStringList fullArgs = valgrindArguments();
    fullArgs << QString::fromLatin1("--tool=%1").arg(tool());
    if (Utils::HostOsInfo::isMacHost())
        // May be slower to start but without it we get no filenames for symbols.
        fullArgs << QLatin1String("--dsymutil=yes");
    return fullArgs;
}

void ValgrindRunner::setDebuggee(const StandardRunnable &debuggee)
{
    d->debuggee = debuggee;
}

void ValgrindRunner::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    d->channelMode = mode;
}

void ValgrindRunner::setDevice(const IDevice::ConstPtr &device)
{
    d->device = device;
}

IDevice::ConstPtr ValgrindRunner::device() const
{
    return d->device;
}

void ValgrindRunner::waitForFinished() const
{
    if (d->finished || !d->process)
        return;

    QEventLoop loop;
    connect(this, &ValgrindRunner::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

bool ValgrindRunner::start()
{
    d->process = new ValgrindProcess(d->device, this);
    d->process->setProcessChannelMode(d->channelMode);
    // consider appending our options last so they override any interfering user-supplied options
    // -q as suggested by valgrind manual
    d->process->setValgrindExecutable(d->valgrindExecutable);
    d->process->setValgrindArguments(fullValgrindArguments());
    d->process->setDebuggee(d->debuggee);

    QObject::connect(d->process, &ValgrindProcess::processOutput,
                     this, &ValgrindRunner::processOutputReceived);
    QObject::connect(d->process, &ValgrindProcess::started,
                     this, &ValgrindRunner::started);
    QObject::connect(d->process, &ValgrindProcess::finished,
                     this, &ValgrindRunner::processFinished);
    QObject::connect(d->process, &ValgrindProcess::error,
                     this, &ValgrindRunner::processError);
    QObject::connect(d->process, &ValgrindProcess::localHostAddressRetrieved,
                     this, &ValgrindRunner::localHostAddressRetrieved);

    d->process->run(d->debuggee.runMode);
    return true;
}

void ValgrindRunner::processError(QProcess::ProcessError e)
{
    if (d->finished)
        return;

    d->finished = true;

    // make sure we don't wait for the connection anymore
    emit processErrorReceived(errorString(), e);
    emit finished();
}

void ValgrindRunner::processFinished(int ret, QProcess::ExitStatus status)
{
    if (d->finished)
        return;

    d->finished = true;

    // make sure we don't wait for the connection anymore
    emit finished();

    if (ret != 0 || status == QProcess::CrashExit)
        emit processErrorReceived(errorString(), d->process->processError());
}

void ValgrindRunner::localHostAddressRetrieved(const QHostAddress &localHostAddress)
{
    Q_UNUSED(localHostAddress);
}

QString ValgrindRunner::errorString() const
{
    if (d->process)
        return d->process->errorString();
    else
        return QString();
}

void ValgrindRunner::stop()
{
    QTC_ASSERT(d->process, finished(); return);
    d->process->close();
}

ValgrindProcess *ValgrindRunner::valgrindProcess() const
{
    return d->process;
}

} // namespace Valgrind
