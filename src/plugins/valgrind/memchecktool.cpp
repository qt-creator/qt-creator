/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "memchecktool.h"

#include "memcheckerrorview.h"
#include "valgrindsettings.h"
#include "valgrindplugin.h"
#include "valgrindengine.h"
#include "valgrindsettings.h"
#include "valgrindrunner.h"

#include "xmlprotocol/error.h"
#include "xmlprotocol/error.h"
#include "xmlprotocol/errorlistmodel.h"
#include "xmlprotocol/frame.h"
#include "xmlprotocol/stack.h"
#include "xmlprotocol/stackmodel.h"
#include "xmlprotocol/status.h"
#include "xmlprotocol/suppression.h"
#include "xmlprotocol/threadedparser.h"

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/analyzer/analyzerconstants.h>
#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/startremotedialog.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>

#include <ssh/sshconnection.h>

#include <utils/fancymainwindow.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHostAddress>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QSortFilterProxyModel>

using namespace Core;
using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;
using namespace Valgrind::XmlProtocol;

using namespace std::placeholders;

namespace Valgrind {
namespace Internal {

const char MEMCHECK_RUN_MODE[] = "MemcheckTool.MemcheckRunMode";
const char MEMCHECK_WITH_GDB_RUN_MODE[] = "MemcheckTool.MemcheckWithGdbRunMode";

const char MemcheckPerspectiveId[] = "Memcheck.Perspective";
const char MemcheckErrorDockId[] = "Memcheck.Dock.Error";


class MemcheckToolRunner : public ValgrindToolRunner
{
    Q_OBJECT

public:
    explicit MemcheckToolRunner(ProjectExplorer::RunControl *runControl,
                                bool withGdb = false);

    void start() override;
    void stop() override;

    QStringList suppressionFiles() const;

signals:
    void internalParserError(const QString &errorString);
    void parserError(const Valgrind::XmlProtocol::Error &error);
    void suppressionCount(const QString &name, qint64 count);

private:
    QString progressTitle() const override;
    QStringList toolArguments() const override;

    void startDebugger(qint64 valgrindPid);
    void appendLog(const QByteArray &data);

    const bool m_withGdb;
    QHostAddress m_localServerAddress;
};

class LocalAddressFinder : public RunWorker
{
public:
    LocalAddressFinder(RunControl *runControl, QHostAddress *localServerAddress)
        : RunWorker(runControl), connection(device()->sshParameters())
    {
        connect(&connection, &QSsh::SshConnection::connected, this, [this, localServerAddress] {
            *localServerAddress = connection.connectionInfo().localAddress;
            reportStarted();
        });
        connect(&connection, &QSsh::SshConnection::error, this, [this] {
            reportFailure();
        });
    }

    void start() override
    {
        connection.connectToHost();
    }

    QSsh::SshConnection connection;
};

MemcheckToolRunner::MemcheckToolRunner(RunControl *runControl, bool withGdb)
    : ValgrindToolRunner(runControl),
      m_withGdb(withGdb),
      m_localServerAddress(QHostAddress::LocalHost)
{
    setDisplayName("MemcheckToolRunner");
    connect(m_runner.parser(), &XmlProtocol::ThreadedParser::error,
            this, &MemcheckToolRunner::parserError);
    connect(m_runner.parser(), &XmlProtocol::ThreadedParser::suppressionCount,
            this, &MemcheckToolRunner::suppressionCount);

    if (withGdb) {
        connect(&m_runner, &ValgrindRunner::valgrindStarted,
                this, &MemcheckToolRunner::startDebugger);
        connect(&m_runner, &ValgrindRunner::logMessageReceived,
                this, &MemcheckToolRunner::appendLog);
//        m_runner.disableXml();
    } else {
        connect(m_runner.parser(), &XmlProtocol::ThreadedParser::internalError,
                this, &MemcheckToolRunner::internalParserError);
    }

    // We need a real address to connect to from the outside.
    if (device()->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
        addStartDependency(new LocalAddressFinder(runControl, &m_localServerAddress));
}

QString MemcheckToolRunner::progressTitle() const
{
    return tr("Analyzing Memory");
}

void MemcheckToolRunner::start()
{
    m_runner.setLocalServerAddress(m_localServerAddress);
    ValgrindToolRunner::start();
}

void MemcheckToolRunner::stop()
{
    disconnect(m_runner.parser(), &ThreadedParser::internalError,
               this, &MemcheckToolRunner::internalParserError);
    ValgrindToolRunner::stop();
}

QStringList MemcheckToolRunner::toolArguments() const
{
    QStringList arguments = {"--tool=memcheck", "--gen-suppressions=all"};

    QTC_ASSERT(m_settings, return arguments);

    if (m_settings->trackOrigins())
        arguments << "--track-origins=yes";

    if (m_settings->showReachable())
        arguments << "--show-reachable=yes";

    QString leakCheckValue;
    switch (m_settings->leakCheckOnFinish()) {
    case ValgrindBaseSettings::LeakCheckOnFinishNo:
        leakCheckValue = "no";
        break;
    case ValgrindBaseSettings::LeakCheckOnFinishYes:
        leakCheckValue = "full";
        break;
    case ValgrindBaseSettings::LeakCheckOnFinishSummaryOnly:
    default:
        leakCheckValue = "summary";
        break;
    }
    arguments << "--leak-check=" + leakCheckValue;

    foreach (const QString &file, m_settings->suppressionFiles())
        arguments << QString("--suppressions=%1").arg(file);

    arguments << QString("--num-callers=%1").arg(m_settings->numCallers());

    if (m_withGdb)
        arguments << "--vgdb=yes" << "--vgdb-error=0";

    return arguments;
}

QStringList MemcheckToolRunner::suppressionFiles() const
{
    return m_settings->suppressionFiles();
}

void MemcheckToolRunner::startDebugger(qint64 valgrindPid)
{
    auto debugger = new Debugger::DebuggerRunTool(runControl());
    debugger->setStartMode(Debugger::AttachToRemoteServer);
    debugger->setRunControlName(QString("VGdb %1").arg(valgrindPid));
    debugger->setRemoteChannel(QString("| vgdb --pid=%1").arg(valgrindPid));
    debugger->setUseContinueInsteadOfRun(true);
    debugger->addExpectedSignal("SIGTRAP");

    connect(runControl(), &RunControl::stopped, debugger, &RunControl::deleteLater);

    debugger->initiateStart();
}

void MemcheckToolRunner::appendLog(const QByteArray &data)
{
    appendMessage(QString::fromUtf8(data), Utils::StdOutFormat);
}


static ErrorListModel::RelevantFrameFinder makeFrameFinder(const QStringList &projectFiles)
{
    return [projectFiles](const Error &error) {
        const Frame defaultFrame = Frame();
        const QVector<Stack> stacks = error.stacks();
        if (stacks.isEmpty())
            return defaultFrame;
        const Stack &stack = stacks[0];
        const QVector<Frame> frames = stack.frames();
        if (frames.isEmpty())
            return defaultFrame;

        //find the first frame belonging to the project
        if (!projectFiles.isEmpty()) {
            foreach (const Frame &frame, frames) {
                if (frame.directory().isEmpty() || frame.fileName().isEmpty())
                    continue;

                //filepaths can contain "..", clean them:
                const QString f = QFileInfo(frame.filePath()).absoluteFilePath();
                if (projectFiles.contains(f))
                    return frame;
            }
        }

        //if no frame belonging to the project was found, return the first one that is not malloc/new
        foreach (const Frame &frame, frames) {
            if (!frame.functionName().isEmpty() && frame.functionName() != QLatin1String("malloc")
                && !frame.functionName().startsWith(QLatin1String("operator new(")))
            {
                return frame;
            }
        }

        //else fallback to the first frame
        return frames.first();
    };
}


class MemcheckErrorFilterProxyModel : public QSortFilterProxyModel
{
public:
    void setAcceptedKinds(const QList<int> &acceptedKinds);
    void setFilterExternalIssues(bool filter);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    QList<int> m_acceptedKinds;
    bool m_filterExternalIssues = false;
};

void MemcheckErrorFilterProxyModel::setAcceptedKinds(const QList<int> &acceptedKinds)
{
    if (m_acceptedKinds != acceptedKinds) {
        m_acceptedKinds = acceptedKinds;
        invalidateFilter();
    }
}

void MemcheckErrorFilterProxyModel::setFilterExternalIssues(bool filter)
{
    if (m_filterExternalIssues != filter) {
        m_filterExternalIssues = filter;
        invalidateFilter();
    }
}

bool MemcheckErrorFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // We only deal with toplevel items.
    if (sourceParent.isValid())
        return true;

    // Because toplevel items have no parent, we can't use sourceParent to find them. we just use
    // sourceParent as an invalid index, telling the model that the index we're looking for has no
    // parent.
    QAbstractItemModel *model = sourceModel();
    QModelIndex sourceIndex = model->index(sourceRow, filterKeyColumn(), sourceParent);
    if (!sourceIndex.isValid())
        return true;

    const Error error = sourceIndex.data(ErrorListModel::ErrorRole).value<Error>();

    // Filter on kind
    if (!m_acceptedKinds.contains(error.kind()))
        return false;

    // Filter non-project stuff
    if (m_filterExternalIssues && !error.stacks().isEmpty()) {
        // ALGORITHM: look at last five stack frames, if none of these is inside any open projects,
        // assume this error was created by an external library
        QSet<QString> validFolders;
        for (Project *project : SessionManager::projects()) {
            validFolders << project->projectDirectory().toString();
            foreach (Target *target, project->targets()) {
                foreach (const DeployableFile &file,
                         target->deploymentData().allFiles()) {
                    if (file.isExecutable())
                        validFolders << file.remoteDirectory();
                }
                foreach (BuildConfiguration *config, target->buildConfigurations())
                    validFolders << config->buildDirectory().toString();
            }
        }

        const QVector< Frame > frames = error.stacks().first().frames();

        const int framesToLookAt = qMin(6, frames.size());

        bool inProject = false;
        for (int i = 0; i < framesToLookAt; ++i) {
            const Frame &frame = frames.at(i);
            foreach (const QString &folder, validFolders) {
                if (frame.directory().startsWith(folder)) {
                    inProject = true;
                    break;
                }
            }
        }
        if (!inProject)
            return false;
    }

    return true;
}

static void initKindFilterAction(QAction *action, const QVariantList &kinds)
{
    action->setCheckable(true);
    action->setData(kinds);
}

class MemcheckTool : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(Valgrind::Internal::MemcheckTool)

public:
    MemcheckTool();

    RunWorker *createRunWorker(RunControl *runControl);

private:
    void updateRunActions();
    void settingsDestroyed(QObject *settings);
    void maybeActiveRunConfigurationChanged();

    void engineFinished();
    void loadingExternalXmlLogFileFinished();

    void parserError(const Valgrind::XmlProtocol::Error &error);
    void internalParserError(const QString &errorString);
    void updateErrorFilter();

    void loadExternalXmlLogFile();

    void setBusyCursor(bool busy);

    void clearErrorView();
    void updateFromSettings();
    int updateUiAfterFinishedHelper();

private:
    ValgrindBaseSettings *m_settings;
    QMenu *m_filterMenu = 0;

    Valgrind::XmlProtocol::ErrorListModel m_errorModel;
    MemcheckErrorFilterProxyModel m_errorProxyModel;
    MemcheckErrorView *m_errorView = 0;

    QList<QAction *> m_errorFilterActions;
    QAction *m_filterProjectAction;
    QList<QAction *> m_suppressionActions;
    QAction *m_startAction;
    QAction *m_startWithGdbAction;
    QAction *m_stopAction;
    QAction *m_suppressionSeparator;
    QAction *m_loadExternalLogFile;
    QAction *m_goBack;
    QAction *m_goNext;
    bool m_toolBusy = false;
};

MemcheckTool::MemcheckTool()
{
    m_settings = ValgrindPlugin::globalSettings();

    setObjectName(QLatin1String("MemcheckTool"));

    m_filterProjectAction = new QAction(tr("External Errors"), this);
    m_filterProjectAction->setToolTip(
        tr("Show issues originating outside currently opened projects."));
    m_filterProjectAction->setCheckable(true);

    m_suppressionSeparator = new QAction(tr("Suppressions"), this);
    m_suppressionSeparator->setSeparator(true);
    m_suppressionSeparator->setToolTip(
        tr("These suppression files were used in the last memory analyzer run."));

    QAction *a = new QAction(tr("Definite Memory Leaks"), this);
    initKindFilterAction(a, {Leak_DefinitelyLost, Leak_IndirectlyLost});
    m_errorFilterActions.append(a);

    a = new QAction(tr("Possible Memory Leaks"), this);
    initKindFilterAction(a, {Leak_PossiblyLost, Leak_StillReachable});
    m_errorFilterActions.append(a);

    a = new QAction(tr("Use of Uninitialized Memory"), this);
    initKindFilterAction(a, {InvalidRead, InvalidWrite, InvalidJump, Overlap,
                             InvalidMemPool, UninitCondition, UninitValue,
                             SyscallParam, ClientCheck});
    m_errorFilterActions.append(a);

    a = new QAction(tr("Invalid Calls to \"free()\""), this);
    initKindFilterAction(a, { InvalidFree,  MismatchedFree });
    m_errorFilterActions.append(a);

    m_errorView = new MemcheckErrorView;
    m_errorView->setObjectName(QLatin1String("MemcheckErrorView"));
    m_errorView->setFrameStyle(QFrame::NoFrame);
    m_errorView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_errorModel.setRelevantFrameFinder(makeFrameFinder(QStringList()));
    m_errorProxyModel.setSourceModel(&m_errorModel);
    m_errorProxyModel.setDynamicSortFilter(true);
    m_errorView->setModel(&m_errorProxyModel);
    m_errorView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    // make m_errorView->selectionModel()->selectedRows() return something
    m_errorView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_errorView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_errorView->setAutoScroll(false);
    m_errorView->setObjectName(QLatin1String("Valgrind.MemcheckTool.ErrorView"));
    m_errorView->setWindowTitle(tr("Memory Issues"));

    Debugger::registerPerspective(MemcheckPerspectiveId, new Perspective (tr("Memcheck"), {
        {MemcheckErrorDockId, m_errorView, {}, Perspective::SplitVertical}
    }));

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::updateRunActions,
            this, &MemcheckTool::maybeActiveRunConfigurationChanged);

    //
    // The Control Widget.
    //

    m_startAction = Debugger::createStartAction();
    m_startWithGdbAction = Debugger::createStartAction();
    m_stopAction = Debugger::createStopAction();

    // Load external XML log file
    auto action = new QAction(this);
    action->setIcon(Icons::OPENFILE.icon());
    action->setToolTip(tr("Load External XML Log File"));
    connect(action, &QAction::triggered, this, &MemcheckTool::loadExternalXmlLogFile);
    m_loadExternalLogFile = action;

    // Go to previous leak.
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Icons::PREV_TOOLBAR.icon());
    action->setToolTip(tr("Go to previous leak."));
    connect(action, &QAction::triggered, m_errorView, &MemcheckErrorView::goBack);
    m_goBack = action;

    // Go to next leak.
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Icons::NEXT_TOOLBAR.icon());
    action->setToolTip(tr("Go to next leak."));
    connect(action, &QAction::triggered, m_errorView, &MemcheckErrorView::goNext);
    m_goNext = action;

    auto filterButton = new QToolButton;
    filterButton->setIcon(Icons::FILTER.icon());
    filterButton->setText(tr("Error Filter"));
    filterButton->setPopupMode(QToolButton::InstantPopup);
    filterButton->setProperty("noArrow", true);

    m_filterMenu = new QMenu(filterButton);
    foreach (QAction *filterAction, m_errorFilterActions)
        m_filterMenu->addAction(filterAction);
    m_filterMenu->addSeparator();
    m_filterMenu->addAction(m_filterProjectAction);
    m_filterMenu->addAction(m_suppressionSeparator);
    connect(m_filterMenu, &QMenu::triggered, this, &MemcheckTool::updateErrorFilter);
    filterButton->setMenu(m_filterMenu);

    ActionContainer *menu = ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);
    QString toolTip = tr("Valgrind Analyze Memory uses the Memcheck tool to find memory leaks.");

    if (!HostOsInfo::isWindowsHost()) {
        action = new QAction(this);
        action->setText(tr("Valgrind Memory Analyzer"));
        action->setToolTip(toolTip);
        menu->addAction(ActionManager::registerAction(action, "Memcheck.Local"),
                        Debugger::Constants::G_ANALYZER_TOOLS);
        QObject::connect(action, &QAction::triggered, this, [action] {
            if (!Debugger::wantRunTool(DebugMode, action->text()))
                return;
            TaskHub::clearTasks(Debugger::Constants::ANALYZERTASK_ID);
            Debugger::selectPerspective(MemcheckPerspectiveId);
            ProjectExplorerPlugin::runStartupProject(MEMCHECK_RUN_MODE);
        });
        QObject::connect(m_startAction, &QAction::triggered, action, &QAction::triggered);
        QObject::connect(m_startAction, &QAction::changed, action, [action, this] {
            action->setEnabled(m_startAction->isEnabled());
        });

        action = new QAction(this);
        action->setText(tr("Valgrind Memory Analyzer with GDB"));
        action->setToolTip(tr("Valgrind Analyze Memory with GDB uses the "
            "Memcheck tool to find memory leaks.\nWhen a problem is detected, "
            "the application is interrupted and can be debugged."));
        menu->addAction(ActionManager::registerAction(action, "MemcheckWithGdb.Local"),
                        Debugger::Constants::G_ANALYZER_TOOLS);
        QObject::connect(action, &QAction::triggered, this, [action] {
            if (!Debugger::wantRunTool(DebugMode, action->text()))
                return;
            TaskHub::clearTasks(Debugger::Constants::ANALYZERTASK_ID);
            Debugger::selectPerspective(MemcheckPerspectiveId);
            ProjectExplorerPlugin::runStartupProject(MEMCHECK_WITH_GDB_RUN_MODE);
        });
        QObject::connect(m_startWithGdbAction, &QAction::triggered, action, &QAction::triggered);
        QObject::connect(m_startWithGdbAction, &QAction::changed, action, [action, this] {
            action->setEnabled(m_startWithGdbAction->isEnabled());
        });
    }

    action = new QAction(this);
    action->setText(tr("Valgrind Memory Analyzer (External Application)"));
    action->setToolTip(toolTip);
    menu->addAction(ActionManager::registerAction(action, "Memcheck.Remote"),
                    Debugger::Constants::G_ANALYZER_REMOTE_TOOLS);
    QObject::connect(action, &QAction::triggered, this, [action] {
        auto runConfig = RunConfiguration::startupRunConfiguration();
        if (!runConfig) {
            showCannotStartDialog(action->text());
            return;
        }
        StartRemoteDialog dlg;
        if (dlg.exec() != QDialog::Accepted)
            return;
        TaskHub::clearTasks(Debugger::Constants::ANALYZERTASK_ID);
        Debugger::selectPerspective(MemcheckPerspectiveId);
        RunControl *rc = new RunControl(runConfig, MEMCHECK_RUN_MODE);
        rc->createWorker(MEMCHECK_RUN_MODE);
        const auto runnable = dlg.runnable();
        rc->setRunnable(runnable);
        rc->setDisplayName(runnable.executable);
        ProjectExplorerPlugin::startRunControl(rc);
    });

    ToolbarDescription toolbar;
    toolbar.addAction(m_startAction);
    //toolbar.addAction(m_startWithGdbAction);
    toolbar.addAction(m_stopAction);
    toolbar.addAction(m_loadExternalLogFile);
    toolbar.addAction(m_goBack);
    toolbar.addAction(m_goNext);
    toolbar.addWidget(filterButton);
    Debugger::registerToolbar(MemcheckPerspectiveId, toolbar);

    updateFromSettings();
    maybeActiveRunConfigurationChanged();
}

void MemcheckTool::updateRunActions()
{
    if (m_toolBusy) {
        m_startAction->setEnabled(false);
        m_startAction->setToolTip(tr("A Valgrind Memcheck analysis is still in progress."));
        m_startWithGdbAction->setEnabled(false);
        m_startWithGdbAction->setToolTip(tr("A Valgrind Memcheck analysis is still in progress."));
        m_stopAction->setEnabled(true);
    } else {
        QString whyNot = tr("Start a Valgrind Memcheck analysis.");
        bool canRun = ProjectExplorerPlugin::canRunStartupProject(MEMCHECK_RUN_MODE, &whyNot);
        m_startAction->setToolTip(whyNot);
        m_startAction->setEnabled(canRun);
        whyNot = tr("Start a Valgrind Memcheck with GDB analysis.");
        canRun = ProjectExplorerPlugin::canRunStartupProject(MEMCHECK_WITH_GDB_RUN_MODE, &whyNot);
        m_startWithGdbAction->setToolTip(whyNot);
        m_startWithGdbAction->setEnabled(canRun);
        m_stopAction->setEnabled(false);
    }
}

void MemcheckTool::settingsDestroyed(QObject *settings)
{
    QTC_ASSERT(m_settings == settings, return);
    m_settings = ValgrindPlugin::globalSettings();
}

void MemcheckTool::updateFromSettings()
{
    foreach (QAction *action, m_errorFilterActions) {
        bool contained = true;
        foreach (const QVariant &v, action->data().toList()) {
            bool ok;
            int kind = v.toInt(&ok);
            if (ok && !m_settings->visibleErrorKinds().contains(kind))
                contained = false;
        }
        action->setChecked(contained);
    }

    m_filterProjectAction->setChecked(!m_settings->filterExternalIssues());
    m_errorView->settingsChanged(m_settings);

    connect(m_settings, &ValgrindBaseSettings::visibleErrorKindsChanged,
            &m_errorProxyModel, &MemcheckErrorFilterProxyModel::setAcceptedKinds);
    m_errorProxyModel.setAcceptedKinds(m_settings->visibleErrorKinds());

    connect(m_settings, &ValgrindBaseSettings::filterExternalIssuesChanged,
            &m_errorProxyModel, &MemcheckErrorFilterProxyModel::setFilterExternalIssues);
    m_errorProxyModel.setFilterExternalIssues(m_settings->filterExternalIssues());
}

void MemcheckTool::maybeActiveRunConfigurationChanged()
{
    updateRunActions();

    ValgrindBaseSettings *settings = 0;
    if (Project *project = SessionManager::startupProject())
        if (Target *target = project->activeTarget())
            if (RunConfiguration *rc = target->activeRunConfiguration())
                if (IRunConfigurationAspect *aspect = rc->extraAspect(ANALYZER_VALGRIND_SETTINGS))
                    settings = qobject_cast<ValgrindBaseSettings *>(aspect->currentSettings());

    if (!settings) // fallback to global settings
        settings = ValgrindPlugin::globalSettings();

    if (m_settings == settings)
        return;

    // disconnect old settings class if any
    if (m_settings) {
        m_settings->disconnect(this);
        m_settings->disconnect(&m_errorProxyModel);
    }

    // now make the new settings current, update and connect input widgets
    m_settings = settings;
    QTC_ASSERT(m_settings, return);
    connect(m_settings, &ValgrindBaseSettings::destroyed, this, &MemcheckTool::settingsDestroyed);

    updateFromSettings();
}

RunWorker *MemcheckTool::createRunWorker(RunControl *runControl)
{
    m_errorModel.setRelevantFrameFinder(makeFrameFinder(runControl->project()->files(Project::AllFiles)));

    auto runTool = new MemcheckToolRunner(runControl, runControl->runMode() == MEMCHECK_WITH_GDB_RUN_MODE);

    connect(runTool, &MemcheckToolRunner::parserError, this, &MemcheckTool::parserError);
    connect(runTool, &MemcheckToolRunner::internalParserError, this, &MemcheckTool::internalParserError);
    connect(runTool, &MemcheckToolRunner::stopped, this, &MemcheckTool::engineFinished);

    m_stopAction->disconnect();
    connect(m_stopAction, &QAction::triggered, runControl, &RunControl::initiateStop);

    m_toolBusy = true;
    updateRunActions();

    setBusyCursor(true);
    clearErrorView();
    m_loadExternalLogFile->setDisabled(true);

    QString dir = runControl->project()->projectDirectory().toString() + '/';
    const QString name = FileName::fromString(runTool->executable()).fileName();

    m_errorView->setDefaultSuppressionFile(dir + name + ".supp");

    foreach (const QString &file, runTool->suppressionFiles()) {
        QAction *action = m_filterMenu->addAction(FileName::fromString(file).fileName());
        action->setToolTip(file);
        connect(action, &QAction::triggered, this, [file] {
            EditorManager::openEditorAt(file, 0);
        });
        m_suppressionActions.append(action);
    }

    return runTool;
}

void MemcheckTool::loadExternalXmlLogFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
                ICore::mainWindow(),
                tr("Open Memcheck XML Log File"),
                QString(),
                tr("XML Files (*.xml);;All Files (*)"));
    if (filePath.isEmpty())
        return;

    QFile *logFile = new QFile(filePath);
    if (!logFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
        delete logFile;
        QString msg = tr("Memcheck: Failed to open file for reading: %1").arg(filePath);
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        return;
    }

    setBusyCursor(true);
    clearErrorView();
    m_loadExternalLogFile->setDisabled(true);

    if (!m_settings || m_settings != ValgrindPlugin::globalSettings()) {
        m_settings = ValgrindPlugin::globalSettings();
        m_errorView->settingsChanged(m_settings);
        updateFromSettings();
    }

    ThreadedParser *parser = new ThreadedParser;
    connect(parser, &ThreadedParser::error, this, &MemcheckTool::parserError);
    connect(parser, &ThreadedParser::internalError, this, &MemcheckTool::internalParserError);
    connect(parser, &ThreadedParser::finished, this, &MemcheckTool::loadingExternalXmlLogFileFinished);
    connect(parser, &ThreadedParser::finished, parser, &ThreadedParser::deleteLater);

    parser->parse(logFile); // ThreadedParser owns the file
}

void MemcheckTool::parserError(const Error &error)
{
    m_errorModel.addError(error);
}

void MemcheckTool::internalParserError(const QString &errorString)
{
    QString msg = tr("Memcheck: Error occurred parsing Valgrind output: %1").arg(errorString);
    TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
    TaskHub::requestPopup();
}

void MemcheckTool::clearErrorView()
{
    QTC_ASSERT(m_errorView, return);
    m_errorModel.clear();

    qDeleteAll(m_suppressionActions);
    m_suppressionActions.clear();
    //QTC_ASSERT(filterMenu()->actions().last() == m_suppressionSeparator, qt_noop());
}

void MemcheckTool::updateErrorFilter()
{
    QTC_ASSERT(m_errorView, return);
    QTC_ASSERT(m_settings, return);

    m_settings->setFilterExternalIssues(!m_filterProjectAction->isChecked());

    QList<int> errorKinds;
    foreach (QAction *a, m_errorFilterActions) {
        if (!a->isChecked())
            continue;
        foreach (const QVariant &v, a->data().toList()) {
            bool ok;
            int kind = v.toInt(&ok);
            if (ok)
                errorKinds << kind;
        }
    }
    m_settings->setVisibleErrorKinds(errorKinds);
}

int MemcheckTool::updateUiAfterFinishedHelper()
{
    const int issuesFound = m_errorModel.rowCount();
    m_goBack->setEnabled(issuesFound > 1);
    m_goNext->setEnabled(issuesFound > 1);
    m_loadExternalLogFile->setEnabled(true);
    setBusyCursor(false);
    return issuesFound;
}

void MemcheckTool::engineFinished()
{
    m_toolBusy = false;
    updateRunActions();

    const int issuesFound = updateUiAfterFinishedHelper();
    Debugger::showPermanentStatusMessage(
        tr("Memory Analyzer Tool finished, %n issues were found.", 0, issuesFound));
}

void MemcheckTool::loadingExternalXmlLogFileFinished()
{
    const int issuesFound = updateUiAfterFinishedHelper();
    Debugger::showPermanentStatusMessage(
        tr("Log file processed, %n issues were found.", 0, issuesFound));
}

void MemcheckTool::setBusyCursor(bool busy)
{
    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_errorView->setCursor(cursor);
}


static MemcheckTool *theMemcheckTool;

void initMemcheckTool()
{
    theMemcheckTool = new MemcheckTool;

    auto producer = std::bind(&MemcheckTool::createRunWorker, theMemcheckTool, _1);
    RunControl::registerWorker(MEMCHECK_RUN_MODE, producer);
    RunControl::registerWorker(MEMCHECK_WITH_GDB_RUN_MODE, producer);
}

void destroyMemcheckTool()
{
    delete theMemcheckTool;
    theMemcheckTool = nullptr;
}

} // namespace Internal
} // namespace Valgrind

#include "memchecktool.moc"
