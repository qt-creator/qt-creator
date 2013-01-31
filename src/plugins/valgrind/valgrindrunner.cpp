/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindrunner.h"
#include "valgrindprocess.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>

#include <QEventLoop>

namespace Valgrind {

class ValgrindRunner::Private
{
public:
    explicit Private(ValgrindRunner *qq)
        : q(qq),
          process(0),
          channelMode(QProcess::SeparateChannels),
          finished(false),
          startMode(Analyzer::StartLocal)
    {
    }

    void run(ValgrindProcess *process);

    ValgrindRunner *q;
    ValgrindProcess *process;
    Utils::Environment environment;
    QProcess::ProcessChannelMode channelMode;
    bool finished;
    QString valgrindExecutable;
    QStringList valgrindArguments;
    QString debuggeeExecutable;
    QString debuggeeArguments;
    QString workingdir;
    Analyzer::StartMode startMode;
    QSsh::SshConnectionParameters connParams;
};

void ValgrindRunner::Private::run(ValgrindProcess *_process)
{
    if (process && process->isRunning()) {
        process->close();
        process->disconnect(q);
        process->deleteLater();
    }

    QTC_ASSERT(_process, return);

    process = _process;

    if (environment.size() > 0)
        process->setEnvironment(environment);

    process->setWorkingDirectory(workingdir);
    process->setProcessChannelMode(channelMode);
    // consider appending our options last so they override any interfering user-supplied options
    // -q as suggested by valgrind manual
    QStringList valgrindArgs = valgrindArguments;
    valgrindArgs << QString::fromLatin1("--tool=%1").arg(q->tool());

    if (Utils::HostOsInfo::isMacHost())
        // May be slower to start but without it we get no filenames for symbols.
        valgrindArgs << QLatin1String("--dsymutil=yes");

    QObject::connect(process, SIGNAL(processOutput(QByteArray,Utils::OutputFormat)),
            q, SIGNAL(processOutputReceived(QByteArray,Utils::OutputFormat)));
    QObject::connect(process, SIGNAL(started()),
            q, SLOT(processStarted()));
    QObject::connect(process, SIGNAL(finished(int,QProcess::ExitStatus)),
            q, SLOT(processFinished(int,QProcess::ExitStatus)));
    QObject::connect(process, SIGNAL(error(QProcess::ProcessError)),
            q, SLOT(processError(QProcess::ProcessError)));

    process->run(valgrindExecutable, valgrindArgs, debuggeeExecutable, debuggeeArguments);
}

ValgrindRunner::ValgrindRunner(QObject *parent)
    : QObject(parent),
      d(new Private(this))
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

QString ValgrindRunner::debuggeeExecutable() const
{
    return d->debuggeeExecutable;
}

void ValgrindRunner::setDebuggeeExecutable(const QString &executable)
{
    d->debuggeeExecutable = executable;
}

QString ValgrindRunner::debuggeeArguments() const
{
    return d->debuggeeArguments;
}

void ValgrindRunner::setDebuggeeArguments(const QString &arguments)
{
    d->debuggeeArguments = arguments;
}

Analyzer::StartMode ValgrindRunner::startMode() const
{
    return d->startMode;
}

void ValgrindRunner::setStartMode(Analyzer::StartMode startMode)
{
    d->startMode = startMode;
}

const QSsh::SshConnectionParameters &ValgrindRunner::connectionParameters() const
{
    return d->connParams;
}

void ValgrindRunner::setConnectionParameters(const QSsh::SshConnectionParameters &connParams)
{
    d->connParams = connParams;
}

void ValgrindRunner::setWorkingDirectory(const QString &path)
{
    d->workingdir = path;
}

QString ValgrindRunner::workingDirectory() const
{
    return d->workingdir;
}

void ValgrindRunner::setEnvironment(const Utils::Environment &environment)
{
    d->environment = environment;
}

void ValgrindRunner::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    d->channelMode = mode;
}

void ValgrindRunner::waitForFinished() const
{
    if (d->finished || !d->process)
        return;

    QEventLoop loop;
    connect(this, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
}

bool ValgrindRunner::start()
{
    if (d->startMode == Analyzer::StartLocal)
        d->run(new LocalValgrindProcess(this));
    else if (d->startMode == Analyzer::StartRemote)
        d->run(new RemoteValgrindProcess(d->connParams, this));
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
        emit processErrorReceived(errorString(), d->process->error());
}

void ValgrindRunner::processStarted()
{
    emit started();
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
    if (d->process)
        d->process->close();
}

ValgrindProcess *ValgrindRunner::valgrindProcess() const
{
    return d->process;
}

} // namespace Valgrind
