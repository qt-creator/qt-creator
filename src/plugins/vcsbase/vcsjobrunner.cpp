/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Brian McGillion & Hugues Delorme
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

#include "vcsjobrunner.h"
#include "vcsbaseconstants.h"
#include "vcsbaseoutputwindow.h"
#include "vcsbaseeditor.h"
#include "vcsbaseplugin.h"

#include <utils/synchronousprocess.h>

#include <QtCore/QMutexLocker>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QString>
#include <QtCore/QDebug>
#include <QtCore/QDir>

using namespace VCSBase;

VCSJob::VCSJob(const QString &workingDir,
               const QStringList &args,
               DataEmitMode emitMode) :
    m_workingDir(workingDir),
    m_arguments(args),
    m_emitRaw(emitMode == RawDataEmitMode),
    m_cookie(),
    m_editor(0),
    m_unixTerminalDisabled(false)
{
}

VCSJob::VCSJob(const QString &workingDir,
               const QStringList &args,
               VCSBase::VCSBaseEditorWidget *editor) :
    m_workingDir(workingDir),
    m_arguments(args),
    m_emitRaw(false),
    m_cookie(),
    m_editor(editor),
    m_unixTerminalDisabled(false)
{
}


VCSJob::DataEmitMode VCSJob::dataEmitMode() const
{
    if (m_emitRaw)
        return RawDataEmitMode;
    else if (displayEditor() != 0)
        return EditorDataEmitMode;
    else
        return NoDataEmitMode;
}

VCSBase::VCSBaseEditorWidget *VCSJob::displayEditor() const
{
    return m_editor;
}

QStringList VCSJob::arguments() const
{
    return m_arguments;
}

QString VCSJob::workingDirectory() const
{
    return m_workingDir;
}

const QVariant &VCSJob::cookie() const
{
    return m_cookie;
}

bool VCSJob::unixTerminalDisabled() const
{
    return m_unixTerminalDisabled;
}

void VCSJob::setDisplayEditor(VCSBase::VCSBaseEditorWidget *editor)
{
    m_editor = editor;
    m_emitRaw = false;
}

void VCSJob::setCookie(const QVariant &cookie)
{
    m_cookie = cookie;
}

void VCSJob::setUnixTerminalDisabled(bool v)
{
    m_unixTerminalDisabled = v;
}



VCSJobRunner::VCSJobRunner() :
    m_keepRunning(true)
{
    VCSBase::VCSBaseOutputWindow *ow = VCSBase::VCSBaseOutputWindow::instance();
    connect(this, SIGNAL(error(QString)),
            ow, SLOT(appendError(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(commandStarted(QString)),
            ow, SLOT(appendCommand(QString)), Qt::QueuedConnection);
}

VCSJobRunner::~VCSJobRunner()
{
    stop();
}

void VCSJobRunner::stop()
{
    {
        QMutexLocker mutexLocker(&m_mutex); Q_UNUSED(mutexLocker);
        m_keepRunning = false;
        //Create a dummy task to break the cycle
        QSharedPointer<VCSJob> job(0);
        m_jobs.enqueue(job);
        m_waiter.wakeAll();
    }

    wait();
}

void VCSJobRunner::restart()
{
    stop();
    m_mutex.lock();
    m_keepRunning = true;
    m_mutex.unlock();
    start();
}

void VCSJobRunner::enqueueJob(const QSharedPointer<VCSJob> &job)
{
    QMutexLocker mutexLocker(&m_mutex); Q_UNUSED(mutexLocker);
    m_jobs.enqueue(job);
    m_waiter.wakeAll();
}

void VCSJobRunner::run()
{
    forever {
        m_mutex.lock();
        while (m_jobs.count() == 0)
            m_waiter.wait(&m_mutex);

        if (!m_keepRunning) {
            m_jobs.clear();
            m_mutex.unlock();
            return;
        }

        QSharedPointer<VCSJob> job = m_jobs.dequeue();
        m_mutex.unlock();

        task(job);
    }
}

QString VCSJobRunner::msgStartFailed(const QString &binary, const QString &why)
{
    return tr("Unable to start process '%1': %2").
            arg(QDir::toNativeSeparators(binary), why);
}

QString VCSJobRunner::msgTimeout(int timeoutSeconds)
{
    return tr("Timed out after %1s waiting for mercurial process to finish.").arg(timeoutSeconds);
}

// Set environment for a VCS process to run in locale "C". Note that there appears
// to be a bug in some VCSs (like hg) that causes special characters to be garbled
// when running in a different language, which seems to be independent from the encoding.
void VCSJobRunner::setProcessEnvironment(QProcess *p)
{
    if (p == 0)
        return;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    VCSBase::VCSBasePlugin::setProcessEnvironment(&env, false);
    p->setProcessEnvironment(env);
}

void VCSJobRunner::setSettings(const QString &bin,
                               const QStringList &stdArgs,
                               int timeoutMsec)
{
    m_binary = bin;
    m_standardArguments = stdArgs;
    m_timeoutMS = timeoutMsec;
}

void VCSJobRunner::task(const QSharedPointer<VCSJob> &job)
{
    VCSJob *taskData = job.data();

    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();

    switch (taskData->dataEmitMode()) {
    case VCSJob::NoDataEmitMode :
        //Just output the data to the "Version control" output window
        connect(this, SIGNAL(output(QByteArray)), outputWindow, SLOT(appendData(QByteArray)),
                Qt::QueuedConnection);
        break;
    case VCSJob::RawDataEmitMode :
        //Call the job's signal so the Initator of the job can process the data
        //Because the QSharedPointer that holds the VCSJob will go out of scope and hence be deleted
        //we have to block and wait until the signal is delivered
        connect(this, SIGNAL(output(QByteArray)), taskData, SIGNAL(rawData(QByteArray)),
                Qt::BlockingQueuedConnection);
        break;
    case VCSJob::EditorDataEmitMode :
        //An editor has been created to display the data so send it there
        connect(this, SIGNAL(output(QByteArray)),
                taskData->displayEditor(), SLOT(setPlainTextData(QByteArray)),
                Qt::QueuedConnection);
        break;
    }

    const QStringList args = m_standardArguments + taskData->arguments();
    emit commandStarted(VCSBase::VCSBaseOutputWindow::msgExecutionLogEntry(taskData->workingDirectory(), m_binary, args));
    //infom the user of what we are going to try and perform

    if (Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << "Repository root is "
                 << taskData->workingDirectory() << " terminal_disabled"
                 << taskData->unixTerminalDisabled();

    const unsigned processFlags = taskData->unixTerminalDisabled() ?
                unsigned(Utils::SynchronousProcess::UnixTerminalDisabled) :
                unsigned(0);

    QSharedPointer<QProcess> vcsProcess = Utils::SynchronousProcess::createProcess(processFlags);
    vcsProcess->setWorkingDirectory(taskData->workingDirectory());
    VCSJobRunner::setProcessEnvironment(vcsProcess.data());

    vcsProcess->start(m_binary, args);

    if (!vcsProcess->waitForStarted()) {
        emit error(msgStartFailed(m_binary, vcsProcess->errorString()));
        return;
    }

    vcsProcess->closeWriteChannel();

    QByteArray stdOutput;
    QByteArray stdErr;

    if (!Utils::SynchronousProcess::readDataFromProcess(*vcsProcess, m_timeoutMS, &stdOutput, &stdErr, false)) {
        Utils::SynchronousProcess::stopProcess(*vcsProcess);
        emit error(msgTimeout(m_timeoutMS / 1000));
        return;
    }

    if (vcsProcess->exitStatus() == QProcess::NormalExit) {
        /*
          * sometimes success means output is actually on error channel (stderr)
          * e.g. "hg revert" outputs "no changes needed to 'file'" on stderr if file has not changed
          * from revision specified
          */
        if (stdOutput.isEmpty())
            stdOutput = stdErr;
        emit output(stdOutput); // This will clear the diff "Working..." text.
        if (vcsProcess->exitCode() == 0)
            emit taskData->succeeded(taskData->cookie());
        else
            emit error(QString::fromLocal8Bit(stdErr));
    }

    vcsProcess->close();
    //the signal connection is to last only for the duration of a job/task.  next time a new
    //output signal connection must be made
    disconnect(this, SIGNAL(output(QByteArray)), 0, 0);
}
