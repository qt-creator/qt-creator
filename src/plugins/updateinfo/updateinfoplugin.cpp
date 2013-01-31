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

#include "updateinfoplugin.h"
#include "updateinfobutton.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>

#include <qtconcurrentrun.h>

#include <QtPlugin>
#include <QProcess>
#include <QTimer>
#include <QTimerEvent>
#include <QDebug>
#include <QFile>
#include <QThread>
#include <QCoreApplication>
#include <QProcess>
#include <QDomDocument>
#include <QMessageBox>
#include <QMenu>
#include <QFutureWatcher>

namespace {
    static const quint32 OneMinute = 60000;
}

namespace UpdateInfo {
namespace Internal {

class UpdateInfoPluginPrivate
{
public:
    UpdateInfoPluginPrivate()
        : startUpdaterAction(0),
          currentTimerId(0),
          progressUpdateInfoButton(0),
          checkUpdateInfoWatcher(0)
    {}
    ~UpdateInfoPluginPrivate()
    {
    }

    QAction *startUpdaterAction;
    QString updaterProgram;
    QString updaterCheckOnlyArgument;
    QString updaterRunUiArgument;
    int currentTimerId;
    QFuture<QDomDocument> lastCheckUpdateInfoTask;
    QPointer<Core::FutureProgress> updateInfoProgress;
    UpdateInfoButton *progressUpdateInfoButton;
    QFutureWatcher<QDomDocument> *checkUpdateInfoWatcher;
};


UpdateInfoPlugin::UpdateInfoPlugin()
    : d(new UpdateInfoPluginPrivate)
{
}

UpdateInfoPlugin::~UpdateInfoPlugin()
{
    delete d;
}

void UpdateInfoPlugin::startCheckTimer(uint milliseconds)
{
    if (d->currentTimerId != 0)
        stopCurrentCheckTimer();
    d->currentTimerId = startTimer(milliseconds);
}

void UpdateInfoPlugin::stopCurrentCheckTimer()
{
    killTimer(d->currentTimerId);
    d->currentTimerId = 0;
}


/*! Initializes the plugin. Returns true on success.
    Plugins want to register objects with the plugin manager here.

    \a errorMessage can be used to pass an error message to the plugin system,
       if there was any.
*/
bool UpdateInfoPlugin::initialize(const QStringList & /* arguments */, QString *errorMessage)
{
    d->checkUpdateInfoWatcher = new QFutureWatcher<QDomDocument>(this);
    connect(d->checkUpdateInfoWatcher, SIGNAL(finished()), this, SLOT(reactOnUpdaterOutput()));

    QSettings *settings = Core::ICore::settings();
    d->updaterProgram = settings->value(QLatin1String("Updater/Application")).toString();
    d->updaterCheckOnlyArgument = settings->value(QLatin1String("Updater/CheckOnlyArgument")).toString();
    d->updaterRunUiArgument = settings->value(QLatin1String("Updater/RunUiArgument")).toString();

    if (d->updaterProgram.isEmpty()) {
        *errorMessage = tr("Could not determine location of maintenance tool. Please check "
                           "your installation if you did not enable this plugin manually.");
        return false;
    }

    if (!QFile::exists(d->updaterProgram)) {
        *errorMessage = tr("Could not find maintenance tool at '%1'. Check your installation.")
                .arg(d->updaterProgram);
        return false;
    }

    Core::ActionContainer* const helpActionContainer = Core::ActionManager::actionContainer(Core::Constants::M_HELP);
    helpActionContainer->menu()->addAction(tr("Start Updater"), this, SLOT(startUpdaterUiApplication()));

    //wait some time before we want to have the first check
    startCheckTimer(OneMinute / 10);

    return true;
}

QDomDocument UpdateInfoPlugin::checkForUpdates()
{
    if (QThread::currentThread() == QCoreApplication::instance()->thread())
        qWarning() << Q_FUNC_INFO << " Was not designed to run in main/gui thread -> it is using updaterProcess.waitForFinished()";

    //starting
    QProcess updaterProcess;

    updaterProcess.start(d->updaterProgram,
                         QStringList() << d->updaterCheckOnlyArgument);
    updaterProcess.waitForFinished();

    //process return value
    if (updaterProcess.exitStatus() == QProcess::CrashExit) {
        qWarning() << "Get update info application crashed.";
        //return; //maybe there is some output
    }
    QByteArray updaterOutput = updaterProcess.readAllStandardOutput();
    QDomDocument updatesDomDocument;
    updatesDomDocument.setContent(updaterOutput);

    return updatesDomDocument;
}


void UpdateInfoPlugin::reactOnUpdaterOutput()
{
    QDomDocument updatesDomDocument = d->checkUpdateInfoWatcher->result();

    if (updatesDomDocument.isNull() ||
        !updatesDomDocument.firstChildElement().hasChildNodes())
    { // no updates are available
        startCheckTimer(60 * OneMinute);
    } else {
        //added the current almost finished task to the progressmanager
        d->updateInfoProgress = Core::ICore::progressManager()->addTask(
                d->lastCheckUpdateInfoTask, tr("Update"), QLatin1String("Update.GetInfo"), Core::ProgressManager::KeepOnFinish);

        d->updateInfoProgress->setKeepOnFinish(Core::FutureProgress::KeepOnFinish);

        d->progressUpdateInfoButton = new UpdateInfoButton();
        //the old widget is deleted inside this function
        //and the current widget becomes a child of updateInfoProgress
        d->updateInfoProgress->setWidget(d->progressUpdateInfoButton);

        //d->progressUpdateInfoButton->setText(tr("Update")); //we have this information over the progressbar
        connect(d->progressUpdateInfoButton, SIGNAL(released()),
                this, SLOT(startUpdaterUiApplication()));
    }
}

void UpdateInfoPlugin::startUpdaterUiApplication()
{
    QProcess::startDetached(d->updaterProgram, QStringList() << d->updaterRunUiArgument);
    if (!d->updateInfoProgress.isNull())
        d->updateInfoProgress->setKeepOnFinish(Core::FutureProgress::HideOnFinish); //this is fading out the last updateinfo
    startCheckTimer(OneMinute);
}

void UpdateInfoPlugin::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == d->currentTimerId && !d->lastCheckUpdateInfoTask.isRunning())
    {
        stopCurrentCheckTimer();
        d->lastCheckUpdateInfoTask = QtConcurrent::run(this, &UpdateInfoPlugin::checkForUpdates);
        d->checkUpdateInfoWatcher->setFuture(d->lastCheckUpdateInfoTask);
    }
}

void UpdateInfoPlugin::extensionsInitialized()
{
}

} //namespace Internal
} //namespace UpdateInfo

Q_EXPORT_PLUGIN(UpdateInfo::Internal::UpdateInfoPlugin)
