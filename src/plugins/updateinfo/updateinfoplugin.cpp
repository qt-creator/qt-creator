// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "updateinfoplugin.h"
#include "updateinfotr.h"

#include "settingspage.h"
#include "updateinfotools.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/taskprogress.h>
#include <utils/infobar.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QDate>
#include <QLabel>
#include <QLoggingCategory>
#include <QMenu>
#include <QMetaEnum>
#include <QTimer>
#include <QVersionNumber>
#include <QScrollArea>

#include <memory>

Q_LOGGING_CATEGORY(updateLog, "qtc.updateinfo", QtWarningMsg)

const char UpdaterGroup[] = "Updater";
const char MaintenanceToolKey[] = "MaintenanceTool";
const char AutomaticCheckKey[] = "AutomaticCheck";
const char CheckForNewQtVersionsKey[] = "CheckForNewQtVersions";
const char CheckIntervalKey[] = "CheckUpdateInterval";
const char LastCheckDateKey[] = "LastCheckDate";
const char LastMaxQtVersionKey[] = "LastMaxQtVersion";
const quint32 OneMinute = 60000;
const quint32 OneHour = 3600000;
const char InstallUpdates[] = "UpdateInfo.InstallUpdates";
const char InstallQtUpdates[] = "UpdateInfo.InstallQtUpdates";
const char M_MAINTENANCE_TOOL[] = "QtCreator.Menu.Tools.MaintenanceTool";

using namespace Core;
using namespace Tasking;
using namespace Utils;

namespace UpdateInfo {
namespace Internal {

class UpdateInfoPluginPrivate
{
public:
    FilePath m_maintenanceTool;
    std::unique_ptr<TaskTree> m_taskTree;
    QPointer<TaskProgress> m_progress;
    QString m_updateOutput;
    QString m_packagesOutput;
    QTimer *m_checkUpdatesTimer = nullptr;
    struct Settings
    {
        bool automaticCheck = true;
        UpdateInfoPlugin::CheckUpdateInterval checkInterval = UpdateInfoPlugin::WeeklyCheck;
        bool checkForQtVersions = true;
    };
    Settings m_settings;
    QDate m_lastCheckDate;
    QVersionNumber m_lastMaxQtVersion;
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
    if (d->m_taskTree)
        return; // update task is still running (might have been run manually just before)

    if (nextCheckDate().isValid() && nextCheckDate() > QDate::currentDate())
        return; // not a time for check yet

    startCheckForUpdates();
}

void UpdateInfoPlugin::startCheckForUpdates()
{
    if (d->m_taskTree)
        return; // do not trigger while update task is already running

    emit checkForUpdatesRunningChanged(true);

    const auto doSetup = [this](Process &process, const QStringList &args) {
        process.setCommand({d->m_maintenanceTool, args});
        process.setLowPriority();
    };
    const auto doCleanup = [this] {
        d->m_taskTree.release()->deleteLater();
        checkForUpdatesStopped();
    };

    const auto setupUpdate = [doSetup](Process &process) {
        doSetup(process, {"ch", "-g", "*=false,ifw.package.*=true"});
    };
    const auto updateDone = [this](const Process &process) {
        d->m_updateOutput = process.cleanedStdOut();
    };

    QList<GroupItem> tasks { ProcessTask(setupUpdate, updateDone) };
    if (d->m_settings.checkForQtVersions) {
        const auto setupPackages = [doSetup](Process &process) {
            doSetup(process, {"se", "qt[.]qt[0-9][.][0-9]+$", "-g", "*=false,ifw.package.*=true"});
        };
        const auto packagesDone = [this](const Process &process) {
            d->m_packagesOutput = process.cleanedStdOut();
        };
        tasks << ProcessTask(setupPackages, packagesDone);
    }

    d->m_taskTree.reset(new TaskTree(Group{tasks}));
    connect(d->m_taskTree.get(), &TaskTree::done, this, [this, doCleanup] {
        checkForUpdatesFinished();
        doCleanup();
    });
    connect(d->m_taskTree.get(), &TaskTree::errorOccurred, this, doCleanup);
    d->m_progress = new TaskProgress(d->m_taskTree.get());
    d->m_progress->setHalfLifeTimePerTask(30000); // 30 seconds
    d->m_progress->setDisplayName(Tr::tr("Checking for Updates"));
    d->m_progress->setKeepOnFinish(FutureProgress::KeepOnFinishTillUserInteraction);
    d->m_progress->setSubtitleVisibleInStatusBar(true);
    d->m_taskTree->start();
}

void UpdateInfoPlugin::stopCheckForUpdates()
{
    if (!d->m_taskTree)
        return;

    d->m_taskTree.reset();
    checkForUpdatesStopped();
}

void UpdateInfoPlugin::checkForUpdatesStopped()
{
    d->m_updateOutput.clear();
    d->m_packagesOutput.clear();
    emit checkForUpdatesRunningChanged(false);
}

static QString infoTitle(const QList<Update> &updates, const std::optional<QtPackage> &newQt)
{
    static QString blogUrl("href=\"https://www.qt.io/blog/tag/releases\"");
    if (!updates.isEmpty() && newQt) {
        return Tr::tr(
                   "%1 and other updates are available. Check the <a %2>Qt blog</a> for details.")
            .arg(newQt->displayName, blogUrl);
    } else if (newQt) {
        return Tr::tr("%1 is available. Check the <a %2>Qt blog</a> for details.")
            .arg(newQt->displayName, blogUrl);
    }
    return Tr::tr("New updates are available. Start the update?");
}

static void showUpdateInfo(const QList<Update> &updates,
                           const std::optional<QtPackage> &newQt,
                           const std::function<void()> &startUpdater,
                           const std::function<void()> &startPackageManager)
{
    InfoBarEntry info(InstallUpdates, infoTitle(updates, newQt));
    info.addCustomButton(Tr::tr("Open Settings"), [] {
        ICore::infoBar()->removeInfo(InstallQtUpdates);
        ICore::showOptionsDialog(FILTER_OPTIONS_PAGE_ID);
    });
    if (newQt) {
        info.addCustomButton(Tr::tr("Start Package Manager"), [startPackageManager] {
            ICore::infoBar()->removeInfo(InstallQtUpdates);
            startPackageManager();
        });
    } else {
        info.addCustomButton(Tr::tr("Start Update"), [startUpdater] {
            ICore::infoBar()->removeInfo(InstallUpdates);
            startUpdater();
        });
    }
    if (!updates.isEmpty()) {
        info.setDetailsWidgetCreator([updates, newQt] {
            const QString qtText = newQt ? (newQt->displayName + "</li><li>") : QString();
            const QStringList packageNames = Utils::transform(updates, [](const Update &u) {
                if (u.version.isEmpty())
                    return u.name;
                return Tr::tr("%1 (%2)", "Package name and version").arg(u.name, u.version);
            });
            const QString updateText = packageNames.join("</li><li>");
            auto label = new QLabel;
            label->setText("<qt><p>" + Tr::tr("Available updates:") + "<ul><li>"
                           + qtText + updateText + "</li></ul></p></qt>");
            label->setContentsMargins(2, 2, 2, 2);
            auto scrollArea = new QScrollArea;
            scrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
            scrollArea->setWidget(label);
            scrollArea->setFrameShape(QFrame::NoFrame);
            scrollArea->viewport()->setAutoFillBackground(false);
            label->setAutoFillBackground(false);
            return scrollArea;
        });
    }
    ICore::infoBar()->removeInfo(InstallUpdates); // remove any existing notifications
    ICore::infoBar()->unsuppressInfo(InstallUpdates);
    ICore::infoBar()->addInfo(info);
}

void UpdateInfoPlugin::checkForUpdatesFinished()
{
    setLastCheckDate(QDate::currentDate());

    qCDebug(updateLog) << "--- MaintenanceTool output (updates):";
    qCDebug(updateLog) << qPrintable(d->m_updateOutput);
    qCDebug(updateLog) << "--- MaintenanceTool output (packages):";
    qCDebug(updateLog) << qPrintable(d->m_packagesOutput);

    const QList<Update> updates = availableUpdates(d->m_updateOutput);
    const QList<QtPackage> qtPackages = availableQtPackages(d->m_packagesOutput);
    if (updateLog().isDebugEnabled()) {
        qCDebug(updateLog) << "--- Available updates:";
        for (const Update &u : updates)
            qCDebug(updateLog) << u.name << u.version;
        qCDebug(updateLog) << "--- Available Qt packages:";
        for (const QtPackage &p : qtPackages) {
            qCDebug(updateLog) << p.displayName << p.version << "installed:" << p.installed
                               << "prerelease:" << p.isPrerelease;
        }
    }
    std::optional<QtPackage> qtToNag = qtToNagAbout(qtPackages, &d->m_lastMaxQtVersion);

    if (!updates.isEmpty() || qtToNag) {
        // progress details are shown until user interaction for the "no updates" case,
        // so we can show the "No updates found" text, but if we have updates we don't
        // want to keep it around
        if (d->m_progress)
            d->m_progress->setKeepOnFinish(FutureProgress::HideOnFinish);
        emit newUpdatesAvailable(true);
        showUpdateInfo(
            updates, qtToNag, [this] { startUpdater(); }, [this] { startPackageManager(); });
    } else {
        if (d->m_progress)
            d->m_progress->setSubtitle(Tr::tr("No updates found."));
        emit newUpdatesAvailable(false);
    }
}

bool UpdateInfoPlugin::isCheckForUpdatesRunning() const
{
    return d->m_taskTree.get() != nullptr;
}

void UpdateInfoPlugin::extensionsInitialized()
{
    if (isAutomaticCheck())
        QTimer::singleShot(OneMinute, this, &UpdateInfoPlugin::startAutoCheckForUpdates);
}

bool UpdateInfoPlugin::initialize(const QStringList & /* arguments */, QString *errorMessage)
{
    loadSettings();

    if (d->m_maintenanceTool.isEmpty()) {
        *errorMessage = Tr::tr("Could not determine location of maintenance tool. Please check "
            "your installation if you did not enable this plugin manually.");
        return false;
    }

    if (!d->m_maintenanceTool.isExecutableFile()) {
        *errorMessage = Tr::tr("The maintenance tool at \"%1\" is not an executable. Check your installation.")
            .arg(d->m_maintenanceTool.toUserOutput());
        d->m_maintenanceTool.clear();
        return false;
    }

    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &UpdateInfoPlugin::saveSettings);

    (void) new SettingsPage(this);

    auto mtools = ActionManager::actionContainer(Constants::M_TOOLS);
    ActionContainer *mmaintenanceTool = ActionManager::createMenu(M_MAINTENANCE_TOOL);
    mmaintenanceTool->setOnAllDisabledBehavior(ActionContainer::Hide);
    mmaintenanceTool->menu()->setTitle(Tr::tr("Qt Maintenance Tool"));
    mtools->addMenu(mmaintenanceTool);

    QAction *checkForUpdatesAction = new QAction(Tr::tr("Check for Updates"), this);
    checkForUpdatesAction->setMenuRole(QAction::ApplicationSpecificRole);
    Command *checkForUpdatesCommand = ActionManager::registerAction(checkForUpdatesAction,
                                      "Updates.CheckForUpdates");
    connect(checkForUpdatesAction, &QAction::triggered,
            this, &UpdateInfoPlugin::startCheckForUpdates);
    mmaintenanceTool->addAction(checkForUpdatesCommand);

    QAction *startMaintenanceToolAction = new QAction(Tr::tr("Start Maintenance Tool"), this);
    startMaintenanceToolAction->setMenuRole(QAction::ApplicationSpecificRole);
    Command *startMaintenanceToolCommand = ActionManager::registerAction(startMaintenanceToolAction,
                                           "Updates.StartMaintenanceTool");
    connect(startMaintenanceToolAction, &QAction::triggered, this, [this] {
        startMaintenanceTool({});
    });
    mmaintenanceTool->addAction(startMaintenanceToolCommand);

    return true;
}

void UpdateInfoPlugin::loadSettings() const
{
    UpdateInfoPluginPrivate::Settings def;
    QtcSettings *settings = ICore::settings();
    const Key updaterKey = Key(UpdaterGroup) + '/';
    d->m_maintenanceTool = FilePath::fromSettings(settings->value(updaterKey + MaintenanceToolKey));
    d->m_lastCheckDate = settings->value(updaterKey + LastCheckDateKey, QDate()).toDate();
    d->m_settings.automaticCheck
        = settings->value(updaterKey + AutomaticCheckKey, def.automaticCheck).toBool();
    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator(mo->indexOfEnumerator(CheckIntervalKey));
    if (QTC_GUARD(me.isValid())) {
        const QString checkInterval = settings
                                          ->value(updaterKey + CheckIntervalKey,
                                                  me.valueToKey(def.checkInterval))
                                          .toString();
        bool ok = false;
        const int newValue = me.keyToValue(checkInterval.toUtf8(), &ok);
        if (ok)
            d->m_settings.checkInterval = static_cast<CheckUpdateInterval>(newValue);
    }
    const QString lastMaxQtVersionString = settings->value(updaterKey + LastMaxQtVersionKey)
                                               .toString();
    d->m_lastMaxQtVersion = QVersionNumber::fromString(lastMaxQtVersionString);
    d->m_settings.checkForQtVersions
        = settings->value(updaterKey + CheckForNewQtVersionsKey, def.checkForQtVersions).toBool();
}

void UpdateInfoPlugin::saveSettings()
{
    UpdateInfoPluginPrivate::Settings def;
    QtcSettings *settings = ICore::settings();
    settings->beginGroup(UpdaterGroup);
    settings->setValueWithDefault(LastCheckDateKey, d->m_lastCheckDate, QDate());
    settings->setValueWithDefault(AutomaticCheckKey,
                                  d->m_settings.automaticCheck,
                                  def.automaticCheck);
    // Note: don't save MaintenanceToolKey on purpose! This setting may be set only by installer.
    // If creator is run not from installed SDK, the setting can be manually created here:
    // [CREATOR_INSTALLATION_LOCATION]/share/qtcreator/QtProject/QtCreator.ini or
    // [CREATOR_INSTALLATION_LOCATION]/Qt Creator.app/Contents/Resources/QtProject/QtCreator.ini on OS X
    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator(mo->indexOfEnumerator(CheckIntervalKey));
    settings->setValueWithDefault(CheckIntervalKey,
                                  QString::fromUtf8(me.valueToKey(d->m_settings.checkInterval)),
                                  QString::fromUtf8(me.valueToKey(def.checkInterval)));
    settings->setValueWithDefault(LastMaxQtVersionKey, d->m_lastMaxQtVersion.toString());
    settings->setValueWithDefault(CheckForNewQtVersionsKey,
                                  d->m_settings.checkForQtVersions,
                                  def.checkForQtVersions);
    settings->endGroup();
}

bool UpdateInfoPlugin::isAutomaticCheck() const
{
    return d->m_settings.automaticCheck;
}

void UpdateInfoPlugin::setAutomaticCheck(bool on)
{
    if (d->m_settings.automaticCheck == on)
        return;

    d->m_settings.automaticCheck = on;
    if (on)
        startAutoCheckForUpdates();
    else
        stopAutoCheckForUpdates();
}

UpdateInfoPlugin::CheckUpdateInterval UpdateInfoPlugin::checkUpdateInterval() const
{
    return d->m_settings.checkInterval;
}

void UpdateInfoPlugin::setCheckUpdateInterval(UpdateInfoPlugin::CheckUpdateInterval interval)
{
    if (d->m_settings.checkInterval == interval)
        return;

    d->m_settings.checkInterval = interval;
}

bool UpdateInfoPlugin::isCheckingForQtVersions() const
{
    return d->m_settings.checkForQtVersions;
}

void UpdateInfoPlugin::setCheckingForQtVersions(bool on)
{
    d->m_settings.checkForQtVersions = on;
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
    return nextCheckDate(d->m_settings.checkInterval);
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

void UpdateInfoPlugin::startMaintenanceTool(const QStringList &args) const
{
    Process::startDetached(CommandLine{d->m_maintenanceTool, args});
}

void UpdateInfoPlugin::startUpdater() const
{
    startMaintenanceTool({"--updater"});
}

void UpdateInfoPlugin::startPackageManager() const
{
    startMaintenanceTool({"--start-package-manager"});
}

} //namespace Internal
} //namespace UpdateInfo
