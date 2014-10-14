/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "settingspage.h"
#include "updateinfoplugin.h"
#include "updateinfobutton.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/settingsdatabase.h>

#include <qtconcurrentrun.h>

#include <QBasicTimer>
#include <QDomDocument>
#include <QFile>
#include <QFutureWatcher>
#include <QMenu>
#include <QProcess>
#include <QtPlugin>

namespace {
    static const quint32 OneMinute = 60000;
}

using namespace Core;

namespace UpdateInfo {
namespace Internal {

class UpdateInfoPluginPrivate
{
public:
    UpdateInfoPluginPrivate()
        : progressUpdateInfoButton(0),
          checkUpdateInfoWatcher(0),
          m_settingsPage(0)
    {
    }

    QString updaterProgram;
    QString updaterRunUiArgument;
    QString updaterCheckOnlyArgument;

    QFuture<QDomDocument> lastCheckUpdateInfoTask;
    QPointer<FutureProgress> updateInfoProgress;
    UpdateInfoButton *progressUpdateInfoButton;
    QFutureWatcher<QDomDocument> *checkUpdateInfoWatcher;

    QBasicTimer m_timer;
    QDate m_lastDayChecked;
    QTime m_scheduledUpdateTime;
    SettingsPage *m_settingsPage;
};


UpdateInfoPlugin::UpdateInfoPlugin()
    : d(new UpdateInfoPluginPrivate)
{
}

UpdateInfoPlugin::~UpdateInfoPlugin()
{
    d->lastCheckUpdateInfoTask.cancel();
    d->lastCheckUpdateInfoTask.waitForFinished();

    delete d;
}

bool UpdateInfoPlugin::delayedInitialize()
{
    d->checkUpdateInfoWatcher = new QFutureWatcher<QDomDocument>(this);
    connect(d->checkUpdateInfoWatcher, SIGNAL(finished()), this, SLOT(parseUpdates()));

    d->m_timer.start(OneMinute, this);
    return true;
}

void UpdateInfoPlugin::extensionsInitialized()
{
}

bool UpdateInfoPlugin::initialize(const QStringList & /* arguments */, QString *errorMessage)
{
    loadSettings();
    if (d->updaterProgram.isEmpty()) {
        *errorMessage = tr("Could not determine location of maintenance tool. Please check "
            "your installation if you did not enable this plugin manually.");
        return false;
    }

    if (!QFile::exists(d->updaterProgram)) {
        *errorMessage = tr("Could not find maintenance tool at \"%1\". Check your installation.")
            .arg(d->updaterProgram);
        return false;
    }

    d->m_settingsPage = new SettingsPage(this);
    addAutoReleasedObject(d->m_settingsPage);

    ActionContainer *const container = ActionManager::actionContainer(Core::Constants::M_HELP);
    container->menu()->addAction(tr("Start Updater"), this, SLOT(startUpdaterUiApplication()));

    return true;
}

void UpdateInfoPlugin::loadSettings()
{
    QSettings *qs = ICore::settings();
    if (qs->contains(QLatin1String("Updater/Application"))) {
        settingsHelper(qs);
        qs->remove(QLatin1String("Updater"));
        saveSettings(); // update to the new settings location
    } else {
        settingsHelper(ICore::settingsDatabase());
    }
}

void UpdateInfoPlugin::saveSettings()
{
    SettingsDatabase *settings = ICore::settingsDatabase();
    if (settings) {
        settings->beginTransaction();
        settings->beginGroup(QLatin1String("Updater"));
        settings->setValue(QLatin1String("Application"), d->updaterProgram);
        settings->setValue(QLatin1String("LastDayChecked"), d->m_lastDayChecked);
        settings->setValue(QLatin1String("RunUiArgument"), d->updaterRunUiArgument);
        settings->setValue(QLatin1String("CheckOnlyArgument"), d->updaterCheckOnlyArgument);
        settings->setValue(QLatin1String("ScheduledUpdateTime"), d->m_scheduledUpdateTime);
        settings->endGroup();
        settings->endTransaction();
    }
}

QTime UpdateInfoPlugin::scheduledUpdateTime() const
{
    return d->m_scheduledUpdateTime;
}

void UpdateInfoPlugin::setScheduledUpdateTime(const QTime &time)
{
    d->m_scheduledUpdateTime = time;
}

// -- protected

void UpdateInfoPlugin::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == d->m_timer.timerId()) {
        const QDate today = QDate::currentDate();
        if ((d->m_lastDayChecked == today) || (d->lastCheckUpdateInfoTask.isRunning()))
            return; // we checked already or the update task is still running

        bool check = false;
        if (d->m_lastDayChecked <= today.addDays(-2))
            check = true;   // we haven't checked since some days, force check

        if (QTime::currentTime() > d->m_scheduledUpdateTime)
            check = true; // we are behind schedule, force check

        if (check) {
            d->lastCheckUpdateInfoTask = QtConcurrent::run(this, &UpdateInfoPlugin::update);
            d->checkUpdateInfoWatcher->setFuture(d->lastCheckUpdateInfoTask);
        }
    } else {
        // not triggered from our timer
        ExtensionSystem::IPlugin::timerEvent(event);
    }
}

// -- private slots

void UpdateInfoPlugin::parseUpdates()
{
    QDomDocument updatesDomDocument = d->checkUpdateInfoWatcher->result();
    if (updatesDomDocument.isNull() || !updatesDomDocument.firstChildElement().hasChildNodes())
        return;

    // add the finished task to the progress manager
    d->updateInfoProgress
            = ProgressManager::addTask(d->lastCheckUpdateInfoTask, tr("Updates Available"),
                                       "Update.GetInfo", ProgressManager::KeepOnFinish);
    d->updateInfoProgress->setKeepOnFinish(FutureProgress::KeepOnFinish);

    d->progressUpdateInfoButton = new UpdateInfoButton();
    d->updateInfoProgress->setWidget(d->progressUpdateInfoButton);
    connect(d->progressUpdateInfoButton, SIGNAL(released()), this, SLOT(startUpdaterUiApplication()));
}

void UpdateInfoPlugin::startUpdaterUiApplication()
{
    QProcess::startDetached(d->updaterProgram, QStringList() << d->updaterRunUiArgument);
    if (!d->updateInfoProgress.isNull())  //this is fading out the last update info
        d->updateInfoProgress->setKeepOnFinish(FutureProgress::HideOnFinish);
}

// -- private

QDomDocument UpdateInfoPlugin::update()
{
    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        qWarning() << Q_FUNC_INFO << " was not designed to run in main/ gui thread, it is using "
            "QProcess::waitForFinished()";
    }

    // start
    QProcess updater;
    updater.start(d->updaterProgram, QStringList() << d->updaterCheckOnlyArgument);
    while (updater.state() != QProcess::NotRunning) {
        if (!updater.waitForFinished(1000)
                && d->lastCheckUpdateInfoTask.isCanceled()) {
            updater.kill();
            updater.waitForFinished(-1);
            return QDomDocument();
        }
    }

    // process return value
    QDomDocument updates;
    if (updater.exitStatus() != QProcess::CrashExit) {
        d->m_timer.stop();
        updates.setContent(updater.readAllStandardOutput());
        saveSettings(); // force writing out the last update date
    } else {
        qWarning() << "Updater application crashed.";
    }

    d->m_lastDayChecked = QDate::currentDate();
    return updates;
}

template <typename T>
void UpdateInfoPlugin::settingsHelper(T *settings)
{
    settings->beginGroup(QLatin1String("Updater"));

    d->updaterProgram = settings->value(QLatin1String("Application")).toString();
    d->m_lastDayChecked = settings->value(QLatin1String("LastDayChecked"), QDate()).toDate();
    d->updaterRunUiArgument = settings->value(QLatin1String("RunUiArgument"),
        QLatin1String("--updater")).toString();
    d->updaterCheckOnlyArgument = settings->value(QLatin1String("CheckOnlyArgument"),
        QLatin1String("--checkupdates")).toString();
    d->m_scheduledUpdateTime = settings->value(QLatin1String("ScheduledUpdateTime"), QTime(12, 0))
        .toTime();

    settings->endGroup();
}

} //namespace Internal
} //namespace UpdateInfo
