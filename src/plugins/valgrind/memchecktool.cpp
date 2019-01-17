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
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/modemanager.h>

#include <ssh/sshconnection.h>

#include <utils/checkablemessagebox.h>
#include <utils/fancymainwindow.h>
#include <utils/pathchooser.h>
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

#ifdef Q_OS_WIN
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWinEventNotifier>

#include <utils/winutils.h>

#include <windows.h>
#endif

using namespace Core;
using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;
using namespace Valgrind::XmlProtocol;

namespace Valgrind {
namespace Internal {

const char MEMCHECK_RUN_MODE[] = "MemcheckTool.MemcheckRunMode";
const char MEMCHECK_WITH_GDB_RUN_MODE[] = "MemcheckTool.MemcheckWithGdbRunMode";

class MemcheckToolRunner : public ValgrindToolRunner
{
    Q_OBJECT

public:
    explicit MemcheckToolRunner(ProjectExplorer::RunControl *runControl);

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
        connect(&connection, &QSsh::SshConnection::errorOccurred, this, [this] {
            reportFailure();
        });
    }

    void start() override
    {
        connection.connectToHost();
    }

    QSsh::SshConnection connection;
};

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
            if (!frame.functionName().isEmpty() && frame.functionName() != "malloc"
                && !frame.functionName().startsWith("operator new("))
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
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

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

        const QVector<Frame> frames = error.stacks().constFirst().frames();

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
    ~MemcheckTool() override;

    void setupRunner(MemcheckToolRunner *runTool);
    void loadShowXmlLogFile(const QString &filePath, const QString &exitMsg);

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
    void loadXmlLogFile(const QString &filePath);

    void setBusyCursor(bool busy);

    void clearErrorView();
    void updateFromSettings();
    int updateUiAfterFinishedHelper();

    void heobAction();

private:
    ValgrindBaseSettings *m_settings;
    QMenu *m_filterMenu = nullptr;

    Valgrind::XmlProtocol::ErrorListModel m_errorModel;
    MemcheckErrorFilterProxyModel m_errorProxyModel;
    QPointer<MemcheckErrorView> m_errorView;

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

    QString m_exitMsg;
    Perspective m_perspective{"Memcheck.Perspective", tr("Memcheck")};
};

#ifdef Q_OS_WIN
class HeobDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(HeobDialog)

public:
    HeobDialog(QWidget *parent);

    QString arguments() const;
    QString xmlName() const;
    bool attach() const;
    QString path() const;

    void keyPressEvent(QKeyEvent *e) override;

private:
    void updateEnabled();
    void saveOptions();

private:
    QLineEdit *m_xmlEdit = nullptr;
    QComboBox *m_handleExceptionCombo = nullptr;
    QComboBox *m_pageProtectionCombo = nullptr;
    QCheckBox *m_freedProtectionCheck = nullptr;
    QCheckBox *m_breakpointCheck = nullptr;
    QComboBox *m_leakDetailCombo = nullptr;
    QSpinBox *m_leakSizeSpin = nullptr;
    QComboBox *m_leakRecordingCombo = nullptr;
    QCheckBox *m_attachCheck = nullptr;
    QLineEdit *m_extraArgsEdit = nullptr;
    PathChooser *m_pathChooser = nullptr;
};

class HeobData : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(HeobData)

public:
    HeobData(MemcheckTool *mcTool, const QString &xmlPath, Kit *kit, bool attach);
    ~HeobData() override;

    bool createErrorPipe(DWORD heobPid);
    void readExitData();

private:
    void processFinished();

    void sendHeobAttachPid(DWORD pid);
    void debugStarted();
    void debugStopped();

private:
    HANDLE m_errorPipe = INVALID_HANDLE_VALUE;
    OVERLAPPED m_ov;
    unsigned m_data[2];
    QWinEventNotifier *m_processFinishedNotifier = nullptr;
    MemcheckTool *m_mcTool = nullptr;
    QString m_xmlPath;
    Kit *m_kit = nullptr;
    bool m_attach = false;
    RunControl *m_runControl = nullptr;
};
#endif

MemcheckTool::MemcheckTool()
{
    m_settings = ValgrindPlugin::globalSettings();

    setObjectName("MemcheckTool");

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
    m_errorView->setObjectName("MemcheckErrorView");
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
    m_errorView->setObjectName("Valgrind.MemcheckTool.ErrorView");
    m_errorView->setWindowTitle(tr("Memory Issues"));

    m_perspective.addWindow(m_errorView, Perspective::SplitVertical, nullptr);

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
    action->setIcon(Icons::OPENFILE_TOOLBAR.icon());
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
        QObject::connect(action, &QAction::triggered, this, [this, action] {
            if (!Debugger::wantRunTool(DebugMode, action->text()))
                return;
            TaskHub::clearTasks(Debugger::Constants::ANALYZERTASK_ID);
            m_perspective.select();
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
        QObject::connect(action, &QAction::triggered, this, [this, action] {
            if (!Debugger::wantRunTool(DebugMode, action->text()))
                return;
            TaskHub::clearTasks(Debugger::Constants::ANALYZERTASK_ID);
            m_perspective.select();
            ProjectExplorerPlugin::runStartupProject(MEMCHECK_WITH_GDB_RUN_MODE);
        });
        QObject::connect(m_startWithGdbAction, &QAction::triggered, action, &QAction::triggered);
        QObject::connect(m_startWithGdbAction, &QAction::changed, action, [action, this] {
            action->setEnabled(m_startWithGdbAction->isEnabled());
        });
    } else {
        action = new QAction(tr("Heob"), this);
        Core::Command *cmd = Core::ActionManager::registerAction(action, "Memcheck.Local");
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+H")));
        connect(action, &QAction::triggered, this, &MemcheckTool::heobAction);
        menu->addAction(cmd, Debugger::Constants::G_ANALYZER_TOOLS);
        connect(m_startAction, &QAction::changed, action, [action, this] {
            action->setEnabled(m_startAction->isEnabled());
        });
    }

    action = new QAction(this);
    action->setText(tr("Valgrind Memory Analyzer (External Application)"));
    action->setToolTip(toolTip);
    menu->addAction(ActionManager::registerAction(action, "Memcheck.Remote"),
                    Debugger::Constants::G_ANALYZER_REMOTE_TOOLS);
    QObject::connect(action, &QAction::triggered, this, [this, action] {
        auto runConfig = RunConfiguration::startupRunConfiguration();
        if (!runConfig) {
            showCannotStartDialog(action->text());
            return;
        }
        StartRemoteDialog dlg;
        if (dlg.exec() != QDialog::Accepted)
            return;
        TaskHub::clearTasks(Debugger::Constants::ANALYZERTASK_ID);
        m_perspective.select();
        RunControl *rc = new RunControl(runConfig, MEMCHECK_RUN_MODE);
        if (auto creator = RunControl::producer(runConfig, MEMCHECK_RUN_MODE))
            creator(rc);
        const auto runnable = dlg.runnable();
        rc->setRunnable(runnable);
        rc->setDisplayName(runnable.executable);
        ProjectExplorerPlugin::startRunControl(rc);
    });

    m_perspective.addToolBarAction(m_startAction);
    //toolbar.addAction(m_startWithGdbAction);
    m_perspective.addToolBarAction(m_stopAction);
    m_perspective.addToolBarAction(m_loadExternalLogFile);
    m_perspective.addToolBarAction(m_goBack);
    m_perspective.addToolBarAction(m_goNext);
    m_perspective.addToolBarWidget(filterButton);

    updateFromSettings();
    maybeActiveRunConfigurationChanged();
}

MemcheckTool::~MemcheckTool()
{
    delete m_errorView;
}

void MemcheckTool::heobAction()
{
#ifdef Q_OS_WIN
    Runnable sr;
    Abi abi;
    bool hasLocalRc = false;
    Kit *kit = nullptr;
    if (Project *project = SessionManager::startupProject()) {
        if (Target *target = project->activeTarget()) {
            if (RunConfiguration *rc = target->activeRunConfiguration()) {
                kit = target->kit();
                if (kit) {
                    abi = ToolChainKitInformation::targetAbi(kit);

                    const Runnable runnable = rc->runnable();
                    sr = runnable;
                    const IDevice::ConstPtr device = sr.device;
                    hasLocalRc = device && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
                    if (!hasLocalRc)
                        hasLocalRc = DeviceTypeKitInformation::deviceTypeId(kit) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
                }
            }
        }
    }
    if (!hasLocalRc) {
        const QString msg = tr("Heob: No local run configuration available.");
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        return;
    }
    if (abi.architecture() != Abi::X86Architecture
            || abi.os() != Abi::WindowsOS
            || abi.binaryFormat() != Abi::PEFormat
            || (abi.wordWidth() != 32 && abi.wordWidth() != 64)) {
        const QString msg = tr("Heob: No toolchain available.");
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        return;
    }

    QString executable = sr.executable;
    const QString workingDirectory = Utils::FileUtils::normalizePathName(sr.workingDirectory);
    const QString commandLineArguments = sr.commandLineArguments;
    const QStringList envStrings = sr.environment.toStringList();

    // target executable
    if (executable.isEmpty()) {
        const QString msg = tr("Heob: No executable set.");
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        return;
    }
    if (!QFile::exists(executable))
        executable = Utils::HostOsInfo::withExecutableSuffix(executable);
    if (!QFile::exists(executable)) {
        const QString msg = tr("Heob: Cannot find %1.").arg(executable);
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        return;
    }

    // make executable a relative path if possible
    const QString wdSlashed = workingDirectory + '/';
    if (executable.startsWith(wdSlashed, Qt::CaseInsensitive))
        executable.remove(0, wdSlashed.size());

    // heob arguments
    HeobDialog dialog(Core::ICore::mainWindow());
    if (!dialog.exec())
        return;
    const QString heobArguments = dialog.arguments();

    // heob executable
    const QString heob = QString("heob%1.exe").arg(abi.wordWidth());
    const QString heobPath = dialog.path() + '/' + heob;
    if (!QFile::exists(heobPath)) {
        QMessageBox::critical(Core::ICore::mainWindow(), tr("Heob"),
                              tr("The %1 executables must be in the appropriate location.")
                                  .arg("<a href=\"https://github.com/ssbssa/heob/releases\">Heob</a>"));
        return;
    }

    // dwarfstack
    if (abi.osFlavor() == Abi::WindowsMSysFlavor) {
        const QString dwarfstack = QString("dwarfstack%1.dll").arg(abi.wordWidth());
        const QString dwarfstackPath = dialog.path() + '/' + dwarfstack;
        if (!QFile::exists(dwarfstackPath)
                && CheckableMessageBox::doNotShowAgainInformation(
                    Core::ICore::mainWindow(), tr("Heob"),
                    tr("Heob used with MinGW projects needs the %1 DLLs for proper stacktrace resolution.")
                        .arg("<a href=\"https://github.com/ssbssa/dwarfstack/releases\">Dwarfstack</a>"),
                    ICore::settings(), "HeobDwarfstackInfo",
                    QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                    QDialogButtonBox::Ok) != QDialogButtonBox::Ok)
            return;
    }

    // output xml file
    QDir wdDir(workingDirectory);
    const QString xmlPath = wdDir.absoluteFilePath(dialog.xmlName());
    QFile::remove(xmlPath);

    // full command line
    QString arguments = heob + heobArguments + " \"" + executable + '\"';
    if (!commandLineArguments.isEmpty())
        arguments += ' ' + commandLineArguments;
    QByteArray argumentsCopy(reinterpret_cast<const char *>(arguments.utf16()), arguments.size() * 2 + 2);

    // process environment
    QByteArray env;
    void *envPtr = nullptr;
    if (!envStrings.isEmpty()) {
        uint pos = 0;
        for (const QString &par : envStrings) {
            uint parsize = par.size() * 2 + 2;
            env.resize(env.size() + parsize);
            memcpy(env.data() + pos, par.utf16(), parsize);
            pos += parsize;
        }
        env.resize(env.size() + 2);
        env[pos++] = 0;
        env[pos++] = 0;

        envPtr = env.data();
    }

    // heob process
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    if (!CreateProcess(reinterpret_cast<LPCWSTR>(heobPath.utf16()),
                       reinterpret_cast<LPWSTR>(argumentsCopy.data()), NULL, NULL, FALSE,
                       CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED | CREATE_NEW_CONSOLE, envPtr,
                       reinterpret_cast<LPCWSTR>(workingDirectory.utf16()), &si, &pi)) {
        DWORD e = GetLastError();
        const QString msg = tr("Heob: Cannot create %1 process (%2).").arg(heob).arg(qt_error_string(e));
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        return;
    }

    // heob finished signal handler
    auto hd = new HeobData(this, xmlPath, kit, dialog.attach());
    if (!hd->createErrorPipe(pi.dwProcessId)) {
        delete hd;
        hd = nullptr;
    }

    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (hd)
        hd->readExitData();
#endif
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

    ValgrindBaseSettings *settings = nullptr;
    if (Project *project = SessionManager::startupProject())
        if (Target *target = project->activeTarget())
            if (RunConfiguration *rc = target->activeRunConfiguration())
                settings = rc->currentSettings<ValgrindBaseSettings>(ANALYZER_VALGRIND_SETTINGS);

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

void MemcheckTool::setupRunner(MemcheckToolRunner *runTool)
{
    RunControl *runControl = runTool->runControl();
    m_errorModel.setRelevantFrameFinder(makeFrameFinder(transform(runControl->project()->files(Project::AllFiles),
                                                                  &FileName::toString)));

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
}

void MemcheckTool::loadShowXmlLogFile(const QString &filePath, const QString &exitMsg)
{
    clearErrorView();
    m_settings->setFilterExternalIssues(false);
    m_filterProjectAction->setChecked(true);
    m_perspective.select();
    Core::ModeManager::activateMode(Debugger::Constants::MODE_DEBUG);

    m_exitMsg = exitMsg;
    loadXmlLogFile(filePath);
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

    m_exitMsg.clear();
    loadXmlLogFile(filePath);
}

void MemcheckTool::loadXmlLogFile(const QString &filePath)
{
    auto logFile = new QFile(filePath);
    if (!logFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
        delete logFile;
        QString msg = tr("Memcheck: Failed to open file for reading: %1").arg(filePath);
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        if (!m_exitMsg.isEmpty())
            Debugger::showPermanentStatusMessage(m_exitMsg);
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

    auto parser = new ThreadedParser;
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
        tr("Memory Analyzer Tool finished. %n issues were found.", nullptr, issuesFound));
}

void MemcheckTool::loadingExternalXmlLogFileFinished()
{
    const int issuesFound = updateUiAfterFinishedHelper();
    QString statusMessage = tr("Log file processed. %n issues were found.", nullptr, issuesFound);
    if (!m_exitMsg.isEmpty())
        statusMessage += ' ' + m_exitMsg;
    Debugger::showPermanentStatusMessage(statusMessage);
}

void MemcheckTool::setBusyCursor(bool busy)
{
    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_errorView->setCursor(cursor);
}


static MemcheckTool *theMemcheckTool;

MemcheckToolRunner::MemcheckToolRunner(RunControl *runControl)
    : ValgrindToolRunner(runControl),
      m_withGdb(runControl->runMode() == MEMCHECK_WITH_GDB_RUN_MODE),
      m_localServerAddress(QHostAddress::LocalHost)
{
    setId("MemcheckToolRunner");
    connect(m_runner.parser(), &XmlProtocol::ThreadedParser::error,
            this, &MemcheckToolRunner::parserError);
    connect(m_runner.parser(), &XmlProtocol::ThreadedParser::suppressionCount,
            this, &MemcheckToolRunner::suppressionCount);

    if (m_withGdb) {
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

    theMemcheckTool->setupRunner(this);
}

void initMemcheckTool()
{
    theMemcheckTool = new MemcheckTool;

    RunControl::registerWorker<MemcheckToolRunner>(MEMCHECK_RUN_MODE, {});
    RunControl::registerWorker<MemcheckToolRunner>(MEMCHECK_WITH_GDB_RUN_MODE, {});
}

void destroyMemcheckTool()
{
    delete theMemcheckTool;
    theMemcheckTool = nullptr;
}


#ifdef Q_OS_WIN
const char heobXmlC[] = "heob/Xml";
const char heobHandleExceptionC[] = "heob/HandleException";
const char heobPageProtectionC[] = "heob/PageProtection";
const char heobFreedProtectionC[] = "heob/FreedProtection";
const char heobBreakpointC[] = "heob/Breakpoint";
const char heobLeakDetailC[] = "heob/LeakDetail";
const char heobLeakSizeC[] = "heob/LeakSize";
const char heobLeakRecordingC[] = "heob/LeakRecording";
const char heobAttachC[] = "heob/Attach";
const char heobExtraArgsC[] = "heob/ExtraArgs";
const char heobPathC[] = "heob/Path";

static QString upperHexNum(unsigned num)
{
    return QString("%1").arg(num, 8, 16, QChar('0')).toUpper();
}

HeobDialog::HeobDialog(QWidget *parent) :
    QDialog(parent)
{
    QSettings *settings = Core::ICore::settings();
    const QString xml = settings->value(heobXmlC, "leaks.xml").toString();
    int handleException = settings->value(heobHandleExceptionC, 1).toInt();
    int pageProtection = settings->value(heobPageProtectionC, 0).toInt();
    bool freedProtection = settings->value(heobFreedProtectionC, false).toBool();
    bool breakpoint = settings->value(heobBreakpointC, false).toBool();
    int leakDetail = settings->value(heobLeakDetailC, 1).toInt();
    int leakSize = settings->value(heobLeakSizeC, 0).toInt();
    int leakRecording = settings->value(heobLeakRecordingC, 2).toInt();
    bool attach = settings->value(heobAttachC, false).toBool();
    const QString extraArgs = settings->value(heobExtraArgsC).toString();

    QString path = settings->value(heobPathC).toString();
    if (path.isEmpty()) {
        const QString heobPath = QStandardPaths::findExecutable("heob32.exe");
        if (!heobPath.isEmpty())
            path = QFileInfo(heobPath).path();
    }

    auto layout = new QVBoxLayout;
    // disable resizing
    layout->setSizeConstraint(QLayout::SetFixedSize);

    auto xmlLayout = new QHBoxLayout;
    auto xmlLabel = new QLabel(tr("XML output file:"));
    xmlLayout->addWidget(xmlLabel);
    m_xmlEdit = new QLineEdit;
    m_xmlEdit->setText(xml);
    xmlLayout->addWidget(m_xmlEdit);
    layout->addLayout(xmlLayout);

    auto handleExceptionLayout = new QHBoxLayout;
    auto handleExceptionLabel = new QLabel(tr("Handle exceptions:"));
    handleExceptionLayout->addWidget(handleExceptionLabel);
    m_handleExceptionCombo = new QComboBox;
    m_handleExceptionCombo->addItem(tr("Off"));
    m_handleExceptionCombo->addItem(tr("On"));
    m_handleExceptionCombo->addItem(tr("Only"));
    m_handleExceptionCombo->setCurrentIndex(handleException);
    connect(m_handleExceptionCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &HeobDialog::updateEnabled);
    handleExceptionLayout->addWidget(m_handleExceptionCombo);
    layout->addLayout(handleExceptionLayout);

    auto pageProtectionLayout = new QHBoxLayout;
    auto pageProtectionLabel = new QLabel(tr("Page protection:"));
    pageProtectionLayout->addWidget(pageProtectionLabel);
    m_pageProtectionCombo = new QComboBox;
    m_pageProtectionCombo->addItem(tr("Off"));
    m_pageProtectionCombo->addItem(tr("After"));
    m_pageProtectionCombo->addItem(tr("Before"));
    m_pageProtectionCombo->setCurrentIndex(pageProtection);
    connect(m_pageProtectionCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &HeobDialog::updateEnabled);
    pageProtectionLayout->addWidget(m_pageProtectionCombo);
    layout->addLayout(pageProtectionLayout);

    m_freedProtectionCheck = new QCheckBox(tr("Freed memory protection"));
    m_freedProtectionCheck->setChecked(freedProtection);
    layout->addWidget(m_freedProtectionCheck);

    m_breakpointCheck = new QCheckBox(tr("Raise breakpoint exception on error"));
    m_breakpointCheck->setChecked(breakpoint);
    layout->addWidget(m_breakpointCheck);

    auto leakDetailLayout = new QHBoxLayout;
    auto leakDetailLabel = new QLabel(tr("Leak details:"));
    leakDetailLayout->addWidget(leakDetailLabel);
    m_leakDetailCombo = new QComboBox;
    m_leakDetailCombo->addItem(tr("None"));
    m_leakDetailCombo->addItem(tr("Simple"));
    m_leakDetailCombo->addItem(tr("Detect Leak Types"));
    m_leakDetailCombo->addItem(tr("Detect Leak Types (Show Reachable)"));
    m_leakDetailCombo->addItem(tr("Fuzzy Detect Leak Types"));
    m_leakDetailCombo->addItem(tr("Fuzzy Detect Leak Types (Show Reachable)"));
    m_leakDetailCombo->setCurrentIndex(leakDetail);
    connect(m_leakDetailCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &HeobDialog::updateEnabled);
    leakDetailLayout->addWidget(m_leakDetailCombo);
    layout->addLayout(leakDetailLayout);

    auto leakSizeLayout = new QHBoxLayout;
    auto leakSizeLabel = new QLabel(tr("Minimum leak size:"));
    leakSizeLayout->addWidget(leakSizeLabel);
    m_leakSizeSpin = new QSpinBox;
    m_leakSizeSpin->setMinimum(0);
    m_leakSizeSpin->setMaximum(INT_MAX);
    m_leakSizeSpin->setSingleStep(1000);
    m_leakSizeSpin->setValue(leakSize);
    leakSizeLayout->addWidget(m_leakSizeSpin);
    layout->addLayout(leakSizeLayout);

    auto leakRecordingLayout = new QHBoxLayout;
    auto leakRecordingLabel = new QLabel(tr("Control leak recording:"));
    leakRecordingLayout->addWidget(leakRecordingLabel);
    m_leakRecordingCombo = new QComboBox;
    m_leakRecordingCombo->addItem(tr("Off"));
    m_leakRecordingCombo->addItem(tr("On (Start Disabled)"));
    m_leakRecordingCombo->addItem(tr("On (Start Enabled)"));
    m_leakRecordingCombo->setCurrentIndex(leakRecording);
    leakRecordingLayout->addWidget(m_leakRecordingCombo);
    layout->addLayout(leakRecordingLayout);

    m_attachCheck = new QCheckBox(tr("Run with debugger"));
    m_attachCheck->setChecked(attach);
    layout->addWidget(m_attachCheck);

    auto extraArgsLayout = new QHBoxLayout;
    auto extraArgsLabel = new QLabel(tr("Extra arguments:"));
    extraArgsLayout->addWidget(extraArgsLabel);
    m_extraArgsEdit = new QLineEdit;
    m_extraArgsEdit->setText(extraArgs);
    extraArgsLayout->addWidget(m_extraArgsEdit);
    layout->addLayout(extraArgsLayout);

    auto pathLayout = new QHBoxLayout;
    auto pathLabel = new QLabel(tr("Heob path:"));
    pathLabel->setToolTip(tr("The location of heob32.exe and heob64.exe."));
    pathLayout->addWidget(pathLabel);
    m_pathChooser = new PathChooser;
    m_pathChooser->setPath(path);
    pathLayout->addWidget(m_pathChooser);
    layout->addLayout(pathLayout);

    auto saveLayout = new QHBoxLayout;
    saveLayout->addStretch(1);
    auto saveButton = new QToolButton;
    saveButton->setToolTip(tr("Save current settings as default."));
    saveButton->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    connect(saveButton, &QAbstractButton::clicked, this, &HeobDialog::saveOptions);
    saveLayout->addWidget(saveButton);
    layout->addLayout(saveLayout);

    auto okLayout = new QHBoxLayout;
    okLayout->addStretch(1);
    auto okButton = new QPushButton(tr("OK"));
    okButton->setDefault(true);
    connect(okButton, &QAbstractButton::clicked, this, &QDialog::accept);
    okLayout->addWidget(okButton);
    okLayout->addStretch(1);
    layout->addLayout(okLayout);

    setLayout(layout);

    updateEnabled();

    setWindowTitle(tr("Heob"));

    // disable context help button
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

QString HeobDialog::arguments() const
{
    QString args;

    args += " -A";

    const QString xml = xmlName();
    if (!xml.isEmpty())
        args += " -x" + xml;

    int handleException = m_handleExceptionCombo->currentIndex();
    args += QString(" -h%1").arg(handleException);

    int pageProtection = m_pageProtectionCombo->currentIndex();
    args += QString(" -p%1").arg(pageProtection);

    int freedProtection = m_freedProtectionCheck->isChecked() ? 1 : 0;
    args += QString(" -f%1").arg(freedProtection);

    int breakpoint = m_breakpointCheck->isChecked() ? 1 : 0;
    args += QString(" -r%1").arg(breakpoint);

    int leakDetail = m_leakDetailCombo->currentIndex();
    args += QString(" -l%1").arg(leakDetail);

    int leakSize = m_leakSizeSpin->value();
    args += QString(" -z%1").arg(leakSize);

    int leakRecording = m_leakRecordingCombo->currentIndex();
    args += QString(" -k%1").arg(leakRecording);

    const QString extraArgs = m_extraArgsEdit->text();
    if (!extraArgs.isEmpty())
        args += ' ' + extraArgs;

    return args;
}

QString HeobDialog::xmlName() const
{
    return m_xmlEdit->text().replace(' ', '_');
}

bool HeobDialog::attach() const
{
    return m_attachCheck->isChecked();
}

QString HeobDialog::path() const
{
    return m_pathChooser->path();
}

void HeobDialog::keyPressEvent(QKeyEvent *e)
{
    if (e->key() != Qt::Key_F1)
        return QDialog::keyPressEvent(e);

    reject();
    Core::HelpManager::showHelpUrl("qthelp://org.qt-project.qtcreator/doc/creator-heob.html");
}

void HeobDialog::updateEnabled()
{
    bool enableHeob = m_handleExceptionCombo->currentIndex() < 2;
    bool enableLeakDetection = enableHeob && m_leakDetailCombo->currentIndex() > 0;
    bool enablePageProtection = enableHeob && m_pageProtectionCombo->currentIndex() > 0;

    m_leakDetailCombo->setEnabled(enableHeob);
    m_pageProtectionCombo->setEnabled(enableHeob);
    m_breakpointCheck->setEnabled(enableHeob);

    m_leakSizeSpin->setEnabled(enableLeakDetection);
    m_leakRecordingCombo->setEnabled(enableLeakDetection);

    m_freedProtectionCheck->setEnabled(enablePageProtection);
}

void HeobDialog::saveOptions()
{
    QSettings *settings = Core::ICore::settings();
    settings->setValue(heobXmlC, m_xmlEdit->text());
    settings->setValue(heobHandleExceptionC, m_handleExceptionCombo->currentIndex());
    settings->setValue(heobPageProtectionC, m_pageProtectionCombo->currentIndex());
    settings->setValue(heobFreedProtectionC, m_freedProtectionCheck->isChecked());
    settings->setValue(heobBreakpointC, m_breakpointCheck->isChecked());
    settings->setValue(heobLeakDetailC, m_leakDetailCombo->currentIndex());
    settings->setValue(heobLeakSizeC, m_leakSizeSpin->value());
    settings->setValue(heobLeakRecordingC, m_leakRecordingCombo->currentIndex());
    settings->setValue(heobAttachC, m_attachCheck->isChecked());
    settings->setValue(heobExtraArgsC, m_extraArgsEdit->text());
    settings->setValue(heobPathC, m_pathChooser->path());
}

HeobData::HeobData(MemcheckTool *mcTool, const QString &xmlPath, Kit *kit, bool attach)
    : m_ov(), m_data(), m_mcTool(mcTool), m_xmlPath(xmlPath), m_kit(kit), m_attach(attach)
{
}

HeobData::~HeobData()
{
    delete m_processFinishedNotifier;
    if (m_errorPipe != INVALID_HANDLE_VALUE)
        CloseHandle(m_errorPipe);
    if (m_ov.hEvent)
        CloseHandle(m_ov.hEvent);
}

bool HeobData::createErrorPipe(DWORD heobPid)
{
    const QString pipeName = QString("\\\\.\\Pipe\\heob.error.%1").arg(upperHexNum(heobPid));
    DWORD access = m_attach ? PIPE_ACCESS_DUPLEX : PIPE_ACCESS_INBOUND;
    m_errorPipe = CreateNamedPipe(reinterpret_cast<LPCWSTR>(pipeName.utf16()),
                                  access | FILE_FLAG_OVERLAPPED,
                                  PIPE_TYPE_BYTE, 1, 1024, 1024, 0, NULL);
    return m_errorPipe != INVALID_HANDLE_VALUE;
}

void HeobData::readExitData()
{
    m_ov.Offset = m_ov.OffsetHigh = 0;
    m_ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    bool pipeConnected = ConnectNamedPipe(m_errorPipe, &m_ov);
    if (!pipeConnected) {
        DWORD error = GetLastError();
        if (error == ERROR_PIPE_CONNECTED) {
            pipeConnected = true;
        } else if (error == ERROR_IO_PENDING) {
            if (WaitForSingleObject(m_ov.hEvent, 1000) == WAIT_OBJECT_0)
                pipeConnected = true;
            else
                CancelIo(m_errorPipe);
        }
    }
    if (pipeConnected) {
        if (ReadFile(m_errorPipe, m_data, sizeof(m_data), NULL, &m_ov)
                || GetLastError() == ERROR_IO_PENDING) {
            m_processFinishedNotifier = new QWinEventNotifier(m_ov.hEvent);
            connect(m_processFinishedNotifier, &QWinEventNotifier::activated, this, &HeobData::processFinished);
            m_processFinishedNotifier->setEnabled(true);
            return;
        }
    }

    // connection to heob error pipe failed
    delete this;
}

enum
{
    HEOB_OK,
    HEOB_HELP,
    HEOB_BAD_ARG,
    HEOB_PROCESS_FAIL,
    HEOB_WRONG_BITNESS,
    HEOB_PROCESS_KILLED,
    HEOB_NO_CRT,
    HEOB_EXCEPTION,
    HEOB_OUT_OF_MEMORY,
    HEOB_UNEXPECTED_END,
    HEOB_TRACE,
    HEOB_CONSOLE,
    HEOB_PID_ATTACH = 0x10000000,
};

enum
{
    HEOB_CONTROL_NONE,
    HEOB_CONTROL_ATTACH,
};

void HeobData::processFinished()
{
    m_processFinishedNotifier->setEnabled(false);

    QString exitMsg;
    bool needErrorMsg = true;
    DWORD didread;
    if (GetOverlappedResult(m_errorPipe, &m_ov, &didread, TRUE) && didread == sizeof(m_data)) {
        if (m_data[0] >= HEOB_PID_ATTACH) {
            m_runControl = new RunControl(nullptr, ProjectExplorer::Constants::DEBUG_RUN_MODE);
            auto debugger = new DebuggerRunTool(m_runControl, m_kit);
            debugger->setAttachPid(ProcessHandle(m_data[1]));
            debugger->setRunControlName(tr("Process %1").arg(m_data[1]));
            debugger->setInferiorDevice(DeviceKitInformation::device(m_kit));
            debugger->setStartMode(AttachExternal);
            debugger->setCloseMode(DetachAtClose);
            debugger->setContinueAfterAttach(true);
            debugger->setInferiorExecutable(Utils::imageName(m_data[1]));

            connect(m_runControl, &RunControl::started, this, &HeobData::debugStarted);
            connect(m_runControl, &RunControl::stopped, this, &HeobData::debugStopped);
            debugger->startRunControl();
            return;
        }

        switch (m_data[0]) {
        case HEOB_OK:
            exitMsg = tr("Process finished with exit code %1 (0x%2).").arg(m_data[1]).arg(upperHexNum(m_data[1]));
            needErrorMsg = false;
            break;

        case HEOB_BAD_ARG:
            exitMsg = tr("Unknown argument: -%1").arg((char)m_data[1]);
            break;

        case HEOB_PROCESS_FAIL:
            exitMsg = tr("Cannot create target process.");
            if (m_data[1])
                exitMsg += " (" + qt_error_string(m_data[1]) + ')';
            break;

        case HEOB_WRONG_BITNESS:
            exitMsg = tr("Wrong bitness.");
            break;

        case HEOB_PROCESS_KILLED:
            exitMsg = tr("Process killed.");
            break;

        case HEOB_NO_CRT:
            exitMsg = tr("Only works with dynamically linked CRT.");
            break;

        case HEOB_EXCEPTION:
            exitMsg = tr("Process stopped with unhandled exception code 0x%1.").arg(upperHexNum(m_data[1]));
            needErrorMsg = false;
            break;

        case HEOB_OUT_OF_MEMORY:
            exitMsg = tr("Not enough memory to keep track of allocations.");
            break;

        case HEOB_UNEXPECTED_END:
            exitMsg = tr("Application stopped unexpectedly.");
            break;

        case HEOB_CONSOLE:
            exitMsg = tr("Extra console.");
            break;

        case HEOB_HELP:
        case HEOB_TRACE:
            deleteLater();
            return;

        default:
            exitMsg = tr("Unknown exit reason.");
            break;
        }
    } else {
        exitMsg = tr("Heob stopped unexpectedly.");
    }

    if (needErrorMsg) {
        const QString msg = tr("Heob: %1").arg(exitMsg);
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
    } else {
        m_mcTool->loadShowXmlLogFile(m_xmlPath, exitMsg);
    }

    deleteLater();
}

void HeobData::sendHeobAttachPid(DWORD pid)
{
    m_runControl->disconnect(this);

    m_data[0] = HEOB_CONTROL_ATTACH;
    m_data[1] = pid;
    DWORD e = 0;
    if (WriteFile(m_errorPipe, m_data, sizeof(m_data), NULL, &m_ov)
            || (e = GetLastError()) == ERROR_IO_PENDING) {
        DWORD didwrite;
        if (GetOverlappedResult(m_errorPipe, &m_ov, &didwrite, TRUE)) {
            if (didwrite == sizeof(m_data)) {
                if (ReadFile(m_errorPipe, m_data, sizeof(m_data), NULL, &m_ov)
                        || (e = GetLastError()) == ERROR_IO_PENDING) {
                    m_processFinishedNotifier->setEnabled(true);
                    return;
                }
            } else {
                e = ERROR_BAD_LENGTH;
            }
        } else {
            e = GetLastError();
        }
    }

    const QString msg = tr("Heob: Failure in process attach handshake (%1).").arg(qt_error_string(e));
    TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
    TaskHub::requestPopup();
    deleteLater();
}

void HeobData::debugStarted()
{
    sendHeobAttachPid(GetCurrentProcessId());
}

void HeobData::debugStopped()
{
    sendHeobAttachPid(0);
}
#endif

} // namespace Internal
} // namespace Valgrind

#include "memchecktool.moc"
