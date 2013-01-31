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

#include "checkoutjobs.h"

#include "vcsbaseplugin.h"
#include "vcsbaseoutputwindow.h"

#include <QDebug>
#include <QQueue>
#include <QDir>
#include <utils/synchronousprocess.h>
#include <utils/qtcassert.h>

enum { debug = 0 };

/*!
    \class  VcsBase::AbstractCheckoutJob

    \brief Abstract base class for a job creating an initial project checkout.
           It should be something that runs in the background producing log messages.

    \sa VcsBase::BaseCheckoutWizard
*/

namespace VcsBase {

namespace Internal {

// Use a terminal-less process to suppress SSH prompts.
static inline QSharedPointer<QProcess> createProcess()
{
    unsigned flags = 0;
    if (VcsBasePlugin::isSshPromptConfigured())
        flags = Utils::SynchronousProcess::UnixTerminalDisabled;
    return Utils::SynchronousProcess::createProcess(flags);
}

class ProcessCheckoutJobStep
{
public:
    ProcessCheckoutJobStep() {}
    explicit ProcessCheckoutJobStep(const QString &bin,
                                    const QStringList &args,
                                    const QString &workingDir,
                                    QProcessEnvironment env) :
             binary(bin), arguments(args), workingDirectory(workingDir), environment(env) {}

    QString binary;
    QStringList arguments;
    QString workingDirectory;
    QProcessEnvironment environment;
};

class ProcessCheckoutJobPrivate
{
public:
    ProcessCheckoutJobPrivate();

    QSharedPointer<QProcess> process;
    QQueue<ProcessCheckoutJobStep> stepQueue;
    QString binary;
};

ProcessCheckoutJobPrivate::ProcessCheckoutJobPrivate() :
    process(createProcess())
{
}

} // namespace Internal

AbstractCheckoutJob::AbstractCheckoutJob(QObject *parent) :
    QObject(parent)
{
}

/*!
    \class VcsBase::ProcessCheckoutJob

    \brief Convenience implementation of a VcsBase::AbstractCheckoutJob using a QProcess.
*/

ProcessCheckoutJob::ProcessCheckoutJob(QObject *parent) :
    AbstractCheckoutJob(parent),
    d(new Internal::ProcessCheckoutJobPrivate)
{
    connect(d->process.data(), SIGNAL(error(QProcess::ProcessError)), this, SLOT(slotError(QProcess::ProcessError)));
    connect(d->process.data(), SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotFinished(int,QProcess::ExitStatus)));
    connect(d->process.data(), SIGNAL(readyReadStandardOutput()), this, SLOT(slotOutput()));
    d->process->setProcessChannelMode(QProcess::MergedChannels);
    d->process->closeWriteChannel();
}

ProcessCheckoutJob::~ProcessCheckoutJob()
{
    delete d;
}

void ProcessCheckoutJob::addStep(const QString &binary,
                                const QStringList &args,
                                const QString &workingDirectory,
                                const QProcessEnvironment &env)
{
    if (debug)
        qDebug() << "ProcessCheckoutJob::addStep" << binary << args << workingDirectory;
    d->stepQueue.enqueue(Internal::ProcessCheckoutJobStep(binary, args, workingDirectory, env));
}

void ProcessCheckoutJob::slotOutput()
{
    const QByteArray data = d->process->readAllStandardOutput();
    const QString s = QString::fromLocal8Bit(data, data.endsWith('\n') ? data.size() - 1: data.size());
    if (debug)
        qDebug() << s;
    emit output(s);
}

void ProcessCheckoutJob::slotError(QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::FailedToStart:
        emit failed(tr("Unable to start %1: %2").
                    arg(QDir::toNativeSeparators(d->binary), d->process->errorString()));
        break;
    default:
        emit failed(d->process->errorString());
        break;
    }
}

void ProcessCheckoutJob::slotFinished (int exitCode, QProcess::ExitStatus exitStatus)
{
    if (debug)
        qDebug() << "finished" << exitCode << exitStatus;

    switch (exitStatus) {
    case QProcess::NormalExit:
        emit output(tr("The process terminated with exit code %1.").arg(exitCode));
        if (exitCode == 0)
            slotNext();
        else
            emit failed(tr("The process returned exit code %1.").arg(exitCode));
        break;
    case QProcess::CrashExit:
        emit failed(tr("The process terminated in an abnormal way."));
        break;
    }
}

void ProcessCheckoutJob::start()
{
    QTC_ASSERT(!d->stepQueue.empty(), return);
    slotNext();
}

void ProcessCheckoutJob::slotNext()
{
    if (d->stepQueue.isEmpty()) {
        emit succeeded();
        return;
    }
    // Launch next
    const Internal::ProcessCheckoutJobStep step = d->stepQueue.dequeue();
    d->process->setWorkingDirectory(step.workingDirectory);

    // Set up SSH correctly.
    QProcessEnvironment processEnv = step.environment;
    VcsBasePlugin::setProcessEnvironment(&processEnv, false);
    d->process->setProcessEnvironment(processEnv);

    d->binary = step.binary;
    emit output(VcsBaseOutputWindow::msgExecutionLogEntry(step.workingDirectory, d->binary, step.arguments));
    d->process->start(d->binary, step.arguments);
}

void ProcessCheckoutJob::cancel()
{
    if (debug)
        qDebug() << "ProcessCheckoutJob::start";

    emit output(tr("Stopping..."));
    Utils::SynchronousProcess::stopProcess(*d->process);
}

} // namespace VcsBase
