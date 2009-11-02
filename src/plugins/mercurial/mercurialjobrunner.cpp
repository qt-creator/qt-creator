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
#include "mercurialoutputwindow.h"
#include "constants.h"
#include "mercurialsettings.h"

#include <vcsbase/vcsbaseeditor.h>

#include <QtCore/QProcess>
#include <QtCore/QTime>
#include <QtCore/QString>
#include <QtCore/QSettings>

using namespace Mercurial::Internal;
using namespace Mercurial;

HgTask::HgTask(const QString &repositoryRoot, QStringList &arguments, bool emitRaw)
        :   m_repositoryRoot(repositoryRoot),
        arguments(arguments),
        emitRaw(emitRaw),
        editor(0)
{
}

HgTask::HgTask(const QString &repositoryRoot, QStringList &arguments, VCSBase::VCSBaseEditor *editor)
        :   m_repositoryRoot(repositoryRoot),
        arguments(arguments),
        emitRaw(false),
        editor(editor)

{

}


MercurialJobRunner::MercurialJobRunner()
        :   keepRunning(true)
{
    plugin = MercurialPlugin::instance();
    connect(this, SIGNAL(error(const QByteArray &)), plugin->outputPane(), SLOT(append(const QByteArray &)));
    connect(this, SIGNAL(info(const QString &)), plugin->outputPane(), SLOT(append(const QString &)));
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
    MercurialSettings *settings = MercurialPlugin::instance()->settings();
    binary = settings->binary();
    timeout = settings->timeout();
    standardArguments = settings->standardArguments();
}

void MercurialJobRunner::enqueueJob(QSharedPointer<HgTask> &job)
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

void MercurialJobRunner::task(QSharedPointer<HgTask> &job)
{
    HgTask *taskData = job.data();

    if (taskData->shouldEmit())
        //Call the job's signal so the Initator of the job can process the data
        //Because the QSharedPointer that holds the HgTask will go out of scope and hence be deleted
        //we have to block and wait until the signal is delivered
        connect(this, SIGNAL(output(const QByteArray&)), taskData, SIGNAL(rawData(const QByteArray&)),
                Qt::BlockingQueuedConnection);
    else if (taskData->displayEditor())
        //An editor has been created to display the data so send it there
        connect(this, SIGNAL(output(const QByteArray&)), taskData->displayEditor(), SLOT(setPlainTextData(const QByteArray&)));
    else
        //Just output the data to the  Mercurial output window
        connect(this, SIGNAL(output(const QByteArray &)), plugin->outputPane(), SLOT(append(const QByteArray &)));

    QString time = QTime::currentTime().toString(QLatin1String("HH:mm"));
    QString starting = tr("%1 Calling: %2 %3\n").arg(time, "hg", taskData->args().join(" "));

    //infom the user of what we are going to try and perform
    emit info(starting);

    if (Constants::debug)
        qDebug() << Q_FUNC_INFO << "Repository root is " << taskData->repositoryRoot();

    QProcess hgProcess;
    hgProcess.setWorkingDirectory(taskData->repositoryRoot());

    QStringList args = standardArguments;
    args << taskData->args();

    hgProcess.start(binary, args);

    if (!hgProcess.waitForStarted()) {
        QByteArray errorArray(Constants::ERRORSTARTING);
        emit error(errorArray);
        return;
    }

    hgProcess.closeWriteChannel();

    if (!hgProcess.waitForFinished(timeout)) {
        hgProcess.terminate();
        QByteArray errorArray(Constants::TIMEDOUT);
        emit error(errorArray);
        return;
    }

    if ((hgProcess.exitStatus() == QProcess::NormalExit) && (hgProcess.exitCode() == 0)) {
        QByteArray stdout = hgProcess.readAllStandardOutput();
        /*
          * sometimes success means output is actually on error channel (stderr)
          * e.g. "hg revert" outputs "no changes needed to 'file'" on stderr if file has not changed
          * from revision specified
          */
        if (stdout == "")
            stdout = hgProcess.readAllStandardError();
        emit output(stdout);
    } else {
        QByteArray stderr = hgProcess.readAllStandardError();
        emit error(stderr);
    }

    hgProcess.close();
    //the signal connection is to last only for the duration of a job/task.  next time a new
    //output signal connection must be made
    disconnect(this, SIGNAL(output(const QByteArray &)), 0, 0);
}
