/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#include "mercurialjobrunner.h"
#include "mercurialplugin.h"
#include "constants.h"
#include "mercurialsettings.h"

#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/vcsbaseeditor.h>

#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QDebug>

using namespace Mercurial::Internal;
using namespace Mercurial;

HgTask::HgTask(const QString &repositoryRoot,
               const QStringList &arguments,
               bool emitRaw,
               const QVariant &cookie) :
    m_repositoryRoot(repositoryRoot),
    arguments(arguments),
    emitRaw(emitRaw),
    m_cookie(cookie),
    editor(0)

{
}

HgTask::HgTask(const QString &repositoryRoot,
               const QStringList &arguments,
               VCSBase::VCSBaseEditor *editor,
               const QVariant &cookie) :
    m_repositoryRoot(repositoryRoot),
    arguments(arguments),
    emitRaw(false),
    m_cookie(cookie),
    editor(editor)
{
}

void HgTask::emitSucceeded()
{
    emit succeeded(m_cookie);
}

MercurialJobRunner::MercurialJobRunner() :
    plugin(MercurialPlugin::instance()),
    keepRunning(true)
{
    VCSBase::VCSBaseOutputWindow *ow = VCSBase::VCSBaseOutputWindow::instance();
    connect(this, SIGNAL(error(QString)), ow, SLOT(appendError(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(commandStarted(QString)), ow, SLOT(appendCommand(QString)), Qt::QueuedConnection);
}

MercurialJobRunner::~MercurialJobRunner()
{
    stop();
}

void MercurialJobRunner::stop()
{
    mutex.lock();
    keepRunning = false;
    //Create a dummy task to break the cycle
    QSharedPointer<HgTask> job(0);
    jobs.enqueue(job);
    waiter.wakeAll();
    mutex.unlock();

    wait();
}

void MercurialJobRunner::restart()
{
    stop();
    mutex.lock();
    keepRunning = true;
    mutex.unlock();
    start();
}

void MercurialJobRunner::getSettings()
{
    const MercurialSettings &settings = MercurialPlugin::instance()->settings();
    binary = settings.binary();
    timeout = settings.timeoutMilliSeconds();
    standardArguments = settings.standardArguments();
}

void MercurialJobRunner::enqueueJob(const QSharedPointer<HgTask> &job)
{
    mutex.lock();
    jobs.enqueue(job);
    waiter.wakeAll();
    mutex.unlock();
}

void MercurialJobRunner::run()
{
    getSettings();
    forever {
        mutex.lock();
        while (jobs.count() == 0)
            waiter.wait(&mutex);

        if (!keepRunning) {
            jobs.clear();
            mutex.unlock();
            return;
        }

        QSharedPointer<HgTask> job = jobs.dequeue();
        mutex.unlock();

        task(job);
    }
}

QString MercurialJobRunner::msgExecute(const QString &binary, const QStringList &args)
{
    return tr("Executing: %1 %2\n").arg(binary, args.join(QString(QLatin1Char(' '))));
}

QString MercurialJobRunner::msgStartFailed(const QString &binary, const QString &why)
{
    return tr("Unable to start mercurial process '%1': %2").arg(binary, why);
}

QString MercurialJobRunner::msgTimeout(int timeoutSeconds)
{
    return tr("Timed out after %1s waiting for mercurial process to finish.").arg(timeoutSeconds);
}

void MercurialJobRunner::task(const QSharedPointer<HgTask> &job)
{
    HgTask *taskData = job.data();

    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();

    if (taskData->shouldEmit()) {
        //Call the job's signal so the Initator of the job can process the data
        //Because the QSharedPointer that holds the HgTask will go out of scope and hence be deleted
        //we have to block and wait until the signal is delivered
        connect(this, SIGNAL(output(QByteArray)), taskData, SIGNAL(rawData(QByteArray)),
                Qt::BlockingQueuedConnection);
    } else if (taskData->displayEditor()) {
        //An editor has been created to display the data so send it there
        connect(this, SIGNAL(output(QByteArray)),
                taskData->displayEditor(), SLOT(setPlainTextData(QByteArray)),
                Qt::QueuedConnection);
    } else {
        //Just output the data to the  Mercurial output window
        connect(this, SIGNAL(output(QByteArray)), outputWindow, SLOT(appendData(QByteArray)),
                Qt::QueuedConnection);
    }

    const QStringList args = standardArguments + taskData->args();
    emit commandStarted(msgExecute(binary, args));
    //infom the user of what we are going to try and perform

    if (Constants::debug)
        qDebug() << Q_FUNC_INFO << "Repository root is " << taskData->repositoryRoot();

    QProcess hgProcess;
    hgProcess.setWorkingDirectory(taskData->repositoryRoot());


    hgProcess.start(binary, args);

    if (!hgProcess.waitForStarted()) {
        emit error(msgStartFailed(binary, hgProcess.errorString()));
        return;
    }

    hgProcess.closeWriteChannel();

    if (!hgProcess.waitForFinished(timeout)) {
        hgProcess.terminate();
        emit error(msgTimeout(timeout / 1000));
        return;
    }

    if ((hgProcess.exitStatus() == QProcess::NormalExit) && (hgProcess.exitCode() == 0)) {
        QByteArray stdOutput= hgProcess.readAllStandardOutput();
        /*
          * sometimes success means output is actually on error channel (stderr)
          * e.g. "hg revert" outputs "no changes needed to 'file'" on stderr if file has not changed
          * from revision specified
          */
        if (stdOutput.isEmpty())
            stdOutput = hgProcess.readAllStandardError();
        emit output(stdOutput);
        taskData->emitSucceeded();
    } else {
        emit error(QString::fromLocal8Bit(hgProcess.readAllStandardError()));
    }

    hgProcess.close();
    //the signal connection is to last only for the duration of a job/task.  next time a new
    //output signal connection must be made
    disconnect(this, SIGNAL(output(QByteArray)), 0, 0);
}
