// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "updateinfoplugin.h"

#include "settingspage.h"
#include "updateinfoservice.h"
#include "updateinfotools.h"
#include "updateinfotr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/taskprogress.h>

#include <solutions/tasking/tasktreerunner.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/infobar.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLoggingCategory>
#include <QMenu>
#include <QMetaEnum>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVersionNumber>

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
const char M_MAINTENANCE_TOOL[] = "QtCreator.Menu.Tools.MaintenanceTool";

using namespace Core;
using namespace Tasking;
using namespace Utils;

namespace UpdateInfo {
namespace Internal {

class ServiceImpl final : public QObject, public UpdateInfo::Service
{
    Q_OBJECT
    Q_INTERFACES(UpdateInfo::Service)

public:
    bool installPackages(const QString &filterRegex) override;
};

class UpdateInfoPluginPrivate
{
public:
    FilePath m_maintenanceTool;
    TaskTreeRunner m_taskTreeRunner;
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
    std::unique_ptr<ServiceImpl> m_service;
};

static UpdateInfoPluginPrivate *m_d = nullptr;

bool ServiceImpl::installPackages(const QString &filterRegex)
{
    QDialog dialog;
    dialog.setWindowTitle(Tr::tr("Installing Packages"));
    dialog.resize(500, 250);
    auto buttons = new QDialogButtonBox;
    QPushButton *cancelButton = buttons->addButton(QDialogButtonBox::Cancel);
    QPushButton *actionButton = buttons->addButton(QDialogButtonBox::Ok);
    actionButton->setText(Tr::tr("Install"));
    actionButton->setEnabled(false);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto searchProgress = new QProgressBar;
    searchProgress->setRange(0, 0);
    auto installProgress = new QProgressBar;
    installProgress->setRange(0, 0);
    QStackedWidget *stackWidget = nullptr;
    QWidget *notFoundPage = nullptr;
    QWidget *alreadyInstalledPage = nullptr;
    QWidget *packagesPage = nullptr;
    auto packageList = new QLabel;
    QWidget *installPage = nullptr;
    auto installLabel = new QLabel(Tr::tr("Installing..."));
    auto installDetails = new QTextEdit;
    installDetails->setReadOnly(true);
    // clang-format off
    {
        using namespace Layouting;
        Column {
            Stack {
                bindTo(&stackWidget),
                Widget {
                    Column {
                        Tr::tr("Searching for packages..."),
                        searchProgress,
                        st
                    }
                },
                Widget {
                    bindTo(&alreadyInstalledPage),
                    Column {
                        Label {
                            wordWrap(true),
                            text(Tr::tr("All packages matching \"%1\" are already installed.")
                                 .arg(filterRegex))
                        },
                        st
                    }
                },
                Widget {
                    bindTo(&notFoundPage),
                    Column {
                        Label {
                            wordWrap(true),
                            text(Tr::tr("No packages matching \"%1\" were found.").arg(filterRegex))
                        },
                        st
                    }
                },
                Widget {
                    bindTo(&packagesPage),
                    Column {
                        Tr::tr("The following packages were found:"),
                        packageList,
                        st
                    }
                },
                Widget {
                    bindTo(&installPage),
                    Column {
                        installLabel,
                        installProgress,
                        installDetails
                    }
                }
            },
            st,
            buttons
        }.attachTo(&dialog);
    }
    // clang-format on

    TaskTreeRunner runner;
    QList<Package> packages;
    const auto startInstallation = [&dialog,
                                    stackWidget,
                                    installPage,
                                    installLabel,
                                    installProgress,
                                    installDetails,
                                    actionButton,
                                    cancelButton,
                                    &packages,
                                    &runner] {
        stackWidget->setCurrentWidget(installPage);
        actionButton->setEnabled(false);
        disconnect(actionButton, nullptr, &dialog, nullptr);
        const auto onInstallSetup = [&packages, installDetails](Process &process) {
            const QStringList packageIds = Utils::transform(packages, &Package::name);
            const QStringList args = QStringList("in") + packageIds
                                     + QStringList{
                                         "--default-answer",
                                         "--accept-obligations",
                                         "--confirm-command",
                                         "--accept-licenses"};
            process.setCommand({m_d->m_maintenanceTool, args});
            process.setProcessChannelMode(QProcess::MergedChannels);
            process.setStdOutLineCallback([installDetails](QString s) {
                if (s.endsWith('\n'))
                    s.chop(1);
                if (s.endsWith('\r'))
                    s.chop(1);
                installDetails->append(s);
            });
        };
        const auto onInstallDone =
            [&dialog, installLabel, installProgress, actionButton, cancelButton](
                const Process &process) {
                if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
                    installLabel->setText(
                        Tr::tr("An error occurred. Check the output of the installer below."));
                    installProgress->setRange(0, 100);
                    installProgress->setValue(50);
                    return;
                }
                installLabel->setText(Tr::tr("Done."));
                installProgress->setRange(0, 100);
                installProgress->setValue(100);
                actionButton->setText(Tr::tr("Close"));
                actionButton->setEnabled(true);
                connect(actionButton, &QPushButton::clicked, &dialog, &QDialog::accept);
                cancelButton->setVisible(false);
            };
        runner.start({ProcessTask(onInstallSetup, onInstallDone)});
    };
    const auto showNotFoundPage = [stackWidget, notFoundPage, cancelButton] {
        stackWidget->setCurrentWidget(notFoundPage);
        cancelButton->setText(Tr::tr("Close"));
    };
    const auto showAlreadyInstalledPage = [stackWidget, alreadyInstalledPage, cancelButton] {
        stackWidget->setCurrentWidget(alreadyInstalledPage);
        cancelButton->setText(Tr::tr("Close"));
    };
    const auto showPackagesPage = [&dialog,
                                   stackWidget,
                                   packagesPage,
                                   packageList,
                                   actionButton,
                                   &packages,
                                   startInstallation] {
        packageList->setText(
            Utils::transform(packages, [](const Package &p) {
                //: %1 = package name, %2 = package version
                return Tr::tr("%1 (Version: %2)").arg(p.displayName, p.version.toString());
            }).join("<br/>"));
        stackWidget->setCurrentWidget(packagesPage);
        actionButton->setEnabled(true);
        connect(actionButton, &QPushButton::clicked, &dialog, [startInstallation] {
            startInstallation();
        });
    };
    const auto onSearchSetup = [filterRegex](Process &process) {
        qCDebug(updateLog) << "Update service looking for packages matching" << filterRegex;
        process.setCommand(
            {m_d->m_maintenanceTool,
             {"se", filterRegex, "--type=package", "-g", "*=false,ifw.package.*=true"}});
    };
    const auto onSearchDone = [&packages,
                               showNotFoundPage,
                               showAlreadyInstalledPage,
                               showPackagesPage](const Process &process) {
        const QString cleanedStdOut = process.cleanedStdOut();
        qCDebug(updateLog) << "MaintenanceTool output:";
        qCDebug(updateLog) << cleanedStdOut;
        const QList<Package> allAvailablePackages = availablePackages(cleanedStdOut);
        packages = Utils::filtered(allAvailablePackages, [](const Package &p) {
            return p.installedVersion.isNull();
        });
        if (allAvailablePackages.isEmpty())
            showNotFoundPage();
        else if (packages.isEmpty())
            showAlreadyInstalledPage();
        else
            showPackagesPage();
    };

    runner.start({ProcessTask(onSearchSetup, onSearchDone)});

    return dialog.exec() == QDialog::Accepted;
}

UpdateInfoPlugin::UpdateInfoPlugin()
{
    m_d = new UpdateInfoPluginPrivate;
    m_d->m_checkUpdatesTimer = new QTimer(this);
    m_d->m_checkUpdatesTimer->setTimerType(Qt::VeryCoarseTimer);
    m_d->m_checkUpdatesTimer->setInterval(OneHour);
    connect(
        m_d->m_checkUpdatesTimer, &QTimer::timeout, this, &UpdateInfoPlugin::doAutoCheckForUpdates);
}

UpdateInfoPlugin::~UpdateInfoPlugin()
{
    stopCheckForUpdates();
    if (!m_d->m_maintenanceTool.isEmpty())
        saveSettings();

    if (m_d->m_service)
        ExtensionSystem::PluginManager::removeObject(m_d->m_service.get());
    delete m_d;
    m_d = nullptr;
}

void UpdateInfoPlugin::startAutoCheckForUpdates()
{
    doAutoCheckForUpdates();

    m_d->m_checkUpdatesTimer->start();
}

void UpdateInfoPlugin::stopAutoCheckForUpdates()
{
    m_d->m_checkUpdatesTimer->stop();
}

void UpdateInfoPlugin::doAutoCheckForUpdates()
{
    if (m_d->m_taskTreeRunner.isRunning())
        return; // update task is still running (might have been run manually just before)

    if (nextCheckDate().isValid() && nextCheckDate() > QDate::currentDate())
        return; // not a time for check yet

    startCheckForUpdates();
}

void UpdateInfoPlugin::startCheckForUpdates()
{
    if (m_d->m_taskTreeRunner.isRunning())
        return; // do not trigger while update task is already running

    emit checkForUpdatesRunningChanged(true);

    const auto onTreeSetup = [](TaskTree &taskTree) {
        m_d->m_progress = new TaskProgress(&taskTree);
        using namespace std::chrono_literals;
        m_d->m_progress->setHalfLifeTimePerTask(30s);
        m_d->m_progress->setDisplayName(Tr::tr("Checking for Updates"));
        m_d->m_progress->setKeepOnFinish(FutureProgress::KeepOnFinishTillUserInteraction);
        m_d->m_progress->setSubtitleVisibleInStatusBar(true);
    };
    const auto onTreeDone = [this](DoneWith result) {
        if (result == DoneWith::Success)
            checkForUpdatesFinished();
        checkForUpdatesStopped();
    };

    const auto doSetup = [](Process &process, const QStringList &args) {
        process.setCommand({m_d->m_maintenanceTool, args});
        process.setLowPriority();
    };

    const auto onUpdateSetup = [doSetup](Process &process) {
        doSetup(process, {"ch", "-g", "*=false,ifw.package.*=true"});
    };
    const auto onUpdateDone = [](const Process &process) {
        m_d->m_updateOutput = process.cleanedStdOut();
    };

    const Group recipe{
        ProcessTask(onUpdateSetup, onUpdateDone, CallDone::OnSuccess),
        m_d->m_settings.checkForQtVersions
            ? ProcessTask(
                  [doSetup](Process &process) {
                      doSetup(
                          process,
                          {"se", "qt[.]qt[0-9][.][0-9]+$", "-g", "*=false,ifw.package.*=true"});
                  },
                  [](const Process &process) { m_d->m_packagesOutput = process.cleanedStdOut(); },
                  CallDone::OnSuccess)
            : nullItem};
    m_d->m_taskTreeRunner.start(recipe, onTreeSetup, onTreeDone);
}

void UpdateInfoPlugin::stopCheckForUpdates()
{
    if (!m_d->m_taskTreeRunner.isRunning())
        return;

    m_d->m_taskTreeRunner.reset();
    checkForUpdatesStopped();
}

void UpdateInfoPlugin::checkForUpdatesStopped()
{
    m_d->m_updateOutput.clear();
    m_d->m_packagesOutput.clear();
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
    info.setTitle(Tr::tr("Updates Available"));
    info.setInfoType(InfoLabel::Information);
    info.addCustomButton(
        Tr::tr("Open Settings"),
        [] { ICore::showOptionsDialog(FILTER_OPTIONS_PAGE_ID); },
        {},
        InfoBarEntry::ButtonAction::Hide);
    if (newQt) {
        info.addCustomButton(
            Tr::tr("Start Package Manager"),
            [startPackageManager] { startPackageManager(); },
            {},
            InfoBarEntry::ButtonAction::Hide);
    } else {
        info.addCustomButton(
            Tr::tr("Start Update"),
            [startUpdater] { startUpdater(); },
            {},
            InfoBarEntry::ButtonAction::Hide);
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
            //: in the sense "details of the update"
            scrollArea->setWindowTitle(Tr::tr("Update Details"));
            return scrollArea;
        });
    }
    InfoBar *infoBar = ICore::infoBar();
    infoBar->removeInfo(InstallUpdates); // remove any existing notifications
    infoBar->unsuppressInfo(InstallUpdates);
    infoBar->addInfo(info);
}

void UpdateInfoPlugin::checkForUpdatesFinished()
{
    setLastCheckDate(QDate::currentDate());

    qCDebug(updateLog) << "--- MaintenanceTool output (updates):";
    qCDebug(updateLog) << qPrintable(m_d->m_updateOutput);
    qCDebug(updateLog) << "--- MaintenanceTool output (packages):";
    qCDebug(updateLog) << qPrintable(m_d->m_packagesOutput);

    const QList<Update> updates = availableUpdates(m_d->m_updateOutput);
    const QList<QtPackage> qtPackages = availableQtPackages(m_d->m_packagesOutput);
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
    std::optional<QtPackage> qtToNag = qtToNagAbout(qtPackages, &m_d->m_lastMaxQtVersion);

    if (!updates.isEmpty() || qtToNag) {
        // progress details are shown until user interaction for the "no updates" case,
        // so we can show the "No updates found" text, but if we have updates we don't
        // want to keep it around
        if (m_d->m_progress)
            m_d->m_progress->setKeepOnFinish(FutureProgress::HideOnFinish);
        emit newUpdatesAvailable(true);
        showUpdateInfo(
            updates, qtToNag, [this] { startUpdater(); }, [this] { startPackageManager(); });
    } else {
        if (m_d->m_progress)
            m_d->m_progress->setSubtitle(Tr::tr("No updates found."));
        emit newUpdatesAvailable(false);
    }
}

bool UpdateInfoPlugin::isCheckForUpdatesRunning() const
{
    return m_d->m_taskTreeRunner.isRunning();
}

void UpdateInfoPlugin::extensionsInitialized()
{
    if (isAutomaticCheck())
        QTimer::singleShot(OneMinute, this, &UpdateInfoPlugin::startAutoCheckForUpdates);
}

Result<> UpdateInfoPlugin::initialize(const QStringList &)
{
    loadSettings();

    if (m_d->m_maintenanceTool.isEmpty()) {
        return ResultError(Tr::tr("Could not determine location of maintenance tool. Please check "
                                  "your installation if you did not enable this plugin manually."));
    }

    if (!m_d->m_maintenanceTool.isExecutableFile()) {
        m_d->m_maintenanceTool.clear();
        return ResultError(
            Tr::tr("The maintenance tool at \"%1\" is not an executable. Check your installation.")
                .arg(m_d->m_maintenanceTool.toUserOutput()));
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
    m_d->m_service.reset(new ServiceImpl);
    ExtensionSystem::PluginManager::addObject(m_d->m_service.get());

    return ResultOk;
}

void UpdateInfoPlugin::loadSettings() const
{
    UpdateInfoPluginPrivate::Settings def;
    QtcSettings *settings = ICore::settings();
    const Key updaterKey = Key(UpdaterGroup) + '/';
    m_d->m_maintenanceTool = FilePath::fromSettings(
        settings->value(updaterKey + MaintenanceToolKey));
    m_d->m_lastCheckDate = settings->value(updaterKey + LastCheckDateKey, QDate()).toDate();
    m_d->m_settings.automaticCheck
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
            m_d->m_settings.checkInterval = static_cast<CheckUpdateInterval>(newValue);
    }
    const QString lastMaxQtVersionString = settings->value(updaterKey + LastMaxQtVersionKey)
                                               .toString();
    m_d->m_lastMaxQtVersion = QVersionNumber::fromString(lastMaxQtVersionString);
    m_d->m_settings.checkForQtVersions
        = settings->value(updaterKey + CheckForNewQtVersionsKey, def.checkForQtVersions).toBool();
}

void UpdateInfoPlugin::saveSettings()
{
    UpdateInfoPluginPrivate::Settings def;
    QtcSettings *settings = ICore::settings();
    settings->beginGroup(UpdaterGroup);
    settings->setValueWithDefault(LastCheckDateKey, m_d->m_lastCheckDate, QDate());
    settings
        ->setValueWithDefault(AutomaticCheckKey, m_d->m_settings.automaticCheck, def.automaticCheck);
    // Note: don't save MaintenanceToolKey on purpose! This setting may be set only by installer.
    // If creator is run not from installed SDK, the setting can be manually created here:
    // [CREATOR_INSTALLATION_LOCATION]/share/qtcreator/QtProject/QtCreator.ini or
    // [CREATOR_INSTALLATION_LOCATION]/Qt Creator.app/Contents/Resources/QtProject/QtCreator.ini on OS X
    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator(mo->indexOfEnumerator(CheckIntervalKey));
    settings->setValueWithDefault(
        CheckIntervalKey,
        QString::fromUtf8(me.valueToKey(m_d->m_settings.checkInterval)),
        QString::fromUtf8(me.valueToKey(def.checkInterval)));
    settings->setValueWithDefault(LastMaxQtVersionKey, m_d->m_lastMaxQtVersion.toString());
    settings->setValueWithDefault(
        CheckForNewQtVersionsKey, m_d->m_settings.checkForQtVersions, def.checkForQtVersions);
    settings->endGroup();
}

bool UpdateInfoPlugin::isAutomaticCheck() const
{
    return m_d->m_settings.automaticCheck;
}

void UpdateInfoPlugin::setAutomaticCheck(bool on)
{
    if (m_d->m_settings.automaticCheck == on)
        return;

    m_d->m_settings.automaticCheck = on;
    if (on)
        startAutoCheckForUpdates();
    else
        stopAutoCheckForUpdates();
}

UpdateInfoPlugin::CheckUpdateInterval UpdateInfoPlugin::checkUpdateInterval() const
{
    return m_d->m_settings.checkInterval;
}

void UpdateInfoPlugin::setCheckUpdateInterval(UpdateInfoPlugin::CheckUpdateInterval interval)
{
    if (m_d->m_settings.checkInterval == interval)
        return;

    m_d->m_settings.checkInterval = interval;
}

bool UpdateInfoPlugin::isCheckingForQtVersions() const
{
    return m_d->m_settings.checkForQtVersions;
}

void UpdateInfoPlugin::setCheckingForQtVersions(bool on)
{
    m_d->m_settings.checkForQtVersions = on;
}

QDate UpdateInfoPlugin::lastCheckDate() const
{
    return m_d->m_lastCheckDate;
}

void UpdateInfoPlugin::setLastCheckDate(const QDate &date)
{
    if (m_d->m_lastCheckDate == date)
        return;

    m_d->m_lastCheckDate = date;
    emit lastCheckDateChanged(date);
}

QDate UpdateInfoPlugin::nextCheckDate() const
{
    return nextCheckDate(m_d->m_settings.checkInterval);
}

QDate UpdateInfoPlugin::nextCheckDate(CheckUpdateInterval interval) const
{
    if (!m_d->m_lastCheckDate.isValid())
        return QDate();

    if (interval == DailyCheck)
        return m_d->m_lastCheckDate.addDays(1);
    if (interval == WeeklyCheck)
        return m_d->m_lastCheckDate.addDays(7);
    return m_d->m_lastCheckDate.addMonths(1);
}

void UpdateInfoPlugin::startMaintenanceTool(const QStringList &args) const
{
    Process::startDetached(CommandLine{m_d->m_maintenanceTool, args});
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

#include "updateinfoplugin.moc"
