/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "settingspage.h"
#include "updateinfoplugin.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/settingsdatabase.h>
#include <coreplugin/shellcommand.h>
#include <utils/fileutils.h>
#include <utils/synchronousprocess.h>

#include <QDate>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QMetaEnum>
#include <QProcessEnvironment>
#include <QTimer>
#include <QtPlugin>

namespace {
    static const char UpdaterGroup[] = "Updater";
    static const char MaintenanceToolKey[] = "MaintenanceTool";
    static const char AutomaticCheckKey[] = "AutomaticCheck";
    static const char CheckIntervalKey[] = "CheckUpdateInterval";
    static const char LastCheckDateKey[] = "LastCheckDate";
    static const quint32 OneMinute = 60000;
    static const quint32 OneHour = 3600000;
}

using namespace Core;

namespace UpdateInfo {
namespace Internal {

class UpdateInfoPluginPrivate
{
public:
    UpdateInfoPluginPrivate()
    { }

    QString m_maintenanceTool;
    ShellCommand *m_checkUpdatesCommand = 0;
    QString m_collectedOutput;
    QTimer *m_checkUpdatesTimer = 0;

    bool m_automaticCheck = true;
    UpdateInfoPlugin::CheckUpdateInterval m_checkInterval = UpdateInfoPlugin::WeeklyCheck;
    QDate m_lastCheckDate;
};

UpdateInfoPlugin::UpdateInfoPlugin()
    : d(new UpdateInfoPluginPrivate)
{
    d->m_checkUpdatesTimer = new QTimer(this);
    d->m_checkUpdatesTimer->setTimerType(Qt::VeryCoarseTimer);
    d->m_checkUpdatesTimer->setInterval(OneHour);
    connect(d->m_checkUpdatesTimer, &QTimer::timeout,
            this, &UpdateInfoPlugin::doAutoCheckForUpdates);
}

UpdateInfoPlugin::~UpdateInfoPlugin()
{
    stopCheckForUpdates();
    if (!d->m_maintenanceTool.isEmpty())
        saveSettings();

    delete d;
}

void UpdateInfoPlugin::startAutoCheckForUpdates()
{
    doAutoCheckForUpdates();

    d->m_checkUpdatesTimer->start();
}

void UpdateInfoPlugin::stopAutoCheckForUpdates()
{
    d->m_checkUpdatesTimer->stop();
}

void UpdateInfoPlugin::doAutoCheckForUpdates()
{
    if (d->m_checkUpdatesCommand)
        return; // update task is still running (might have been run manually just before)

    if (nextCheckDate().isValid() && nextCheckDate() > QDate::currentDate())
        return; // not a time for check yet

    startCheckForUpdates();
}

void UpdateInfoPlugin::startCheckForUpdates()
{
    stopCheckForUpdates();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QLatin1String("QT_LOGGING_RULES"), QLatin1String("*=false"));
    d->m_checkUpdatesCommand = new ShellCommand(QString(), env);
    connect(d->m_checkUpdatesCommand, &ShellCommand::stdOutText, this, &UpdateInfoPlugin::collectCheckForUpdatesOutput);
    connect(d->m_checkUpdatesCommand, &ShellCommand::finished, this, &UpdateInfoPlugin::checkForUpdatesFinished);
    d->m_checkUpdatesCommand->addJob(Utils::FileName(QFileInfo(d->m_maintenanceTool)), QStringList(QLatin1String("--checkupdates")),
                                     60 * 3, // 3 minutes timeout
                                     /*workingDirectory=*/QString(),
                                     [](int /*exitCode*/) { return Utils::SynchronousProcessResponse::Finished; });
    d->m_checkUpdatesCommand->execute();
    emit checkForUpdatesRunningChanged(true);
}

void UpdateInfoPlugin::stopCheckForUpdates()
{
    if (!d->m_checkUpdatesCommand)
        return;

    d->m_collectedOutput.clear();
    d->m_checkUpdatesCommand->disconnect();
    d->m_checkUpdatesCommand->cancel();
    d->m_checkUpdatesCommand = 0;
    emit checkForUpdatesRunningChanged(false);
}

void UpdateInfoPlugin::collectCheckForUpdatesOutput(const QString &contents)
{
    d->m_collectedOutput += contents;
}

void UpdateInfoPlugin::checkForUpdatesFinished()
{
    setLastCheckDate(QDate::currentDate());

    QDomDocument document;
    document.setContent(d->m_collectedOutput);

    stopCheckForUpdates();

    if (!document.isNull() && document.firstChildElement().hasChildNodes()) {
        emit newUpdatesAvailable(true);
        if (QMessageBox::question(ICore::dialogParent(), tr("Qt Updater"),
                                  tr("New updates are available. Do you want to start the update?"))
                == QMessageBox::Yes)
            startUpdater();
    } else {
        emit newUpdatesAvailable(false);
    }
}

bool UpdateInfoPlugin::isCheckForUpdatesRunning() const
{
    return d->m_checkUpdatesCommand;
}

bool UpdateInfoPlugin::delayedInitialize()
{
    if (isAutomaticCheck())
        QTimer::singleShot(OneMinute, this, &UpdateInfoPlugin::startAutoCheckForUpdates);

    return true;
}

void UpdateInfoPlugin::extensionsInitialized()
{
}

bool UpdateInfoPlugin::initialize(const QStringList & /* arguments */, QString *errorMessage)
{
    loadSettings();

    if (d->m_maintenanceTool.isEmpty()) {
        *errorMessage = tr("Could not determine location of maintenance tool. Please check "
            "your installation if you did not enable this plugin manually.");
        return false;
    }

    if (!QFileInfo(d->m_maintenanceTool).isExecutable()) {
        *errorMessage = tr("The maintenance tool at \"%1\" is not an executable. Check your installation.")
            .arg(d->m_maintenanceTool);
        d->m_maintenanceTool.clear();
        return false;
    }

    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &UpdateInfoPlugin::saveSettings);

    addAutoReleasedObject(new SettingsPage(this));

    QAction *checkForUpdatesAction = new QAction(tr("Check for Updates"), this);
    checkForUpdatesAction->setMenuRole(QAction::ApplicationSpecificRole);
    Core::Command *checkForUpdatesCommand = Core::ActionManager::registerAction(checkForUpdatesAction, "Updates.CheckForUpdates");
    connect(checkForUpdatesAction, &QAction::triggered, this, &UpdateInfoPlugin::startCheckForUpdates);
    ActionContainer *const helpContainer = ActionManager::actionContainer(Core::Constants::M_HELP);
    helpContainer->addAction(checkForUpdatesCommand, Constants::G_HELP_UPDATES);

    return true;
}

void UpdateInfoPlugin::loadSettings() const
{
    QSettings *settings = ICore::settings();
    const QString updaterKey = QLatin1String(UpdaterGroup) + QLatin1Char('/');
    d->m_maintenanceTool = settings->value(updaterKey + QLatin1String(MaintenanceToolKey)).toString();
    d->m_lastCheckDate = settings->value(updaterKey + QLatin1String(LastCheckDateKey), QDate()).toDate();
    d->m_automaticCheck = settings->value(updaterKey + QLatin1String(AutomaticCheckKey), true).toBool();
    const QString checkInterval = settings->value(updaterKey + QLatin1String(CheckIntervalKey)).toString();
    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator(mo->indexOfEnumerator(CheckIntervalKey));
    if (me.isValid()) {
        bool ok = false;
        const int newValue = me.keyToValue(checkInterval.toUtf8(), &ok);
        if (ok)
            d->m_checkInterval = static_cast<CheckUpdateInterval>(newValue);
    }
}

void UpdateInfoPlugin::saveSettings()
{
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String(UpdaterGroup));
    settings->setValue(QLatin1String(LastCheckDateKey), d->m_lastCheckDate);
    settings->setValue(QLatin1String(AutomaticCheckKey), d->m_automaticCheck);
    // Note: don't save MaintenanceToolKey on purpose! This setting may be set only by installer.
    // If creator is run not from installed SDK, the setting can be manually created here:
    // [CREATOR_INSTALLATION_LOCATION]/share/qtcreator/QtProject/QtCreator.ini or
    // [CREATOR_INSTALLATION_LOCATION]/Qt Creator.app/Contents/Resources/QtProject/QtCreator.ini on OS X
    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator(mo->indexOfEnumerator(CheckIntervalKey));
    settings->setValue(QLatin1String(CheckIntervalKey), QLatin1String(me.valueToKey(d->m_checkInterval)));
    settings->endGroup();
}

bool UpdateInfoPlugin::isAutomaticCheck() const
{
    return d->m_automaticCheck;
}

void UpdateInfoPlugin::setAutomaticCheck(bool on)
{
    if (d->m_automaticCheck == on)
        return;

    d->m_automaticCheck = on;
    if (on)
        startAutoCheckForUpdates();
    else
        stopAutoCheckForUpdates();
}

UpdateInfoPlugin::CheckUpdateInterval UpdateInfoPlugin::checkUpdateInterval() const
{
    return d->m_checkInterval;
}

void UpdateInfoPlugin::setCheckUpdateInterval(UpdateInfoPlugin::CheckUpdateInterval interval)
{
    if (d->m_checkInterval == interval)
        return;

    d->m_checkInterval = interval;
}

QDate UpdateInfoPlugin::lastCheckDate() const
{
    return d->m_lastCheckDate;
}

void UpdateInfoPlugin::setLastCheckDate(const QDate &date)
{
    if (d->m_lastCheckDate == date)
        return;

    d->m_lastCheckDate = date;
    emit lastCheckDateChanged(date);
}

QDate UpdateInfoPlugin::nextCheckDate() const
{
    return nextCheckDate(d->m_checkInterval);
}

QDate UpdateInfoPlugin::nextCheckDate(CheckUpdateInterval interval) const
{
    if (!d->m_lastCheckDate.isValid())
        return QDate();

    if (interval == DailyCheck)
        return d->m_lastCheckDate.addDays(1);
    if (interval == WeeklyCheck)
        return d->m_lastCheckDate.addDays(7);
    return d->m_lastCheckDate.addMonths(1);
}

void UpdateInfoPlugin::startUpdater()
{
    QProcess::startDetached(d->m_maintenanceTool, QStringList(QLatin1String("--updater")));
}

} //namespace Internal
} //namespace UpdateInfo
