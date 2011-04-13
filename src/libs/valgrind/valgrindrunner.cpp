/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "valgrindrunner.h"
#include "valgrindprocess.h"

#include <utils/qtcassert.h>

#include <utils/environment.h>

#include <QtCore/QEventLoop>

using namespace Valgrind;

class ValgrindRunner::Private
{
public:
    explicit Private(ValgrindRunner *qq)
        : q(qq),
          process(0),
          channelMode(QProcess::SeparateChannels),
          finished(false),
          lastPid(0)
    {
    }

    void run(Internal::ValgrindProcess *process);

    ValgrindRunner *q;
    Internal::ValgrindProcess *process;
    Utils::Environment environment;
    QProcess::ProcessChannelMode channelMode;
    bool finished;
    Q_PID lastPid;
    QString valgrindExecutable;
    QStringList valgrindArguments;
    QString debuggeeExecutable;
    QString debuggeeArguments;
    QString workingdir;
};

void ValgrindRunner::Private::run(Internal::ValgrindProcess *_process)
{
    if (process && process->isRunning()) {
        process->close();
        process->disconnect(q);
        process->deleteLater();
    }

    lastPid = 0;
    QTC_ASSERT(_process, return);

    process = _process;

    if (environment.size() > 0)
        process->setEnvironment(environment);

    process->setWorkingDirectory(workingdir);
    process->setProcessChannelMode(channelMode);
    // consider appending our options last so they override any interfering user-supplied options
    // -q as suggested by valgrind manual
    QStringList valgrindArgs = valgrindArguments;
    valgrindArgs << QString("--tool=%1").arg(q->tool());

    QObject::connect(process, SIGNAL(standardOutputReceived(QByteArray)),
            q, SIGNAL(standardOutputReceived(QByteArray)));
    QObject::connect(process, SIGNAL(standardErrorReceived(QByteArray)),
            q, SIGNAL(standardErrorReceived(QByteArray)));
    QObject::connect(process, SIGNAL(started()),
            q, SLOT(processStarted()));
    QObject::connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
            q, SLOT(processFinished(int, QProcess::ExitStatus)));
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

void ValgrindRunner::start()
{
    d->run(new Internal::LocalValgrindProcess(this));
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
    if (Internal::LocalValgrindProcess *process = dynamic_cast<Internal::LocalValgrindProcess *>(d->process))
        d->lastPid = process->pid();

    emit started();
}

Q_PID ValgrindRunner::lastPid() const
{
    return d->lastPid;
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
