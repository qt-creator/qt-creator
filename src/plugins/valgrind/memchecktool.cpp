// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "memchecktool.h"

#include "memcheckerrorview.h"
#include "valgrindengine.h"
#include "valgrindprocess.h"
#include "valgrindsettings.h"
#include "valgrindtr.h"

#include "xmlprotocol/error.h"
#include "xmlprotocol/error.h"
#include "xmlprotocol/errorlistmodel.h"
#include "xmlprotocol/frame.h"
#include "xmlprotocol/parser.h"
#include "xmlprotocol/stack.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <debugger/debuggerkitaspect.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/analyzer/analyzerconstants.h>
#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/startremotedialog.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/checkablemessagebox.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QToolButton>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#include <QWinEventNotifier>

#include <utils/winutils.h>

#include <windows.h>
#endif

using namespace Core;
using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;
using namespace Valgrind::XmlProtocol;

namespace Valgrind::Internal {

const char MEMCHECK_RUN_MODE[] = "MemcheckTool.MemcheckRunMode";
const char MEMCHECK_WITH_GDB_RUN_MODE[] = "MemcheckTool.MemcheckWithGdbRunMode";

class MemcheckToolRunner : public ValgrindToolRunner
{
    Q_OBJECT

public:
    explicit MemcheckToolRunner(ProjectExplorer::RunControl *runControl);

    void start() override;
    void stop() override;

    const Utils::FilePaths suppressionFiles() const;

signals:
    void internalParserError(const QString &errorString);
    void parserError(const Valgrind::XmlProtocol::Error &error);

private:
    QString progressTitle() const override;
    void addToolArguments(CommandLine &cmd) const override;

    void startDebugger(qint64 valgrindPid);
    void appendLog(const QByteArray &data);

    const bool m_withGdb;
    QHostAddress m_localServerAddress;
};

class LocalAddressFinder : public RunWorker
{
public:
    LocalAddressFinder(RunControl *runControl, QHostAddress *localServerAddress)
        : RunWorker(runControl), m_localServerAddress(localServerAddress) {}

    void start() override
    {
        QTC_ASSERT(!m_process, return);
        m_process.reset(new Process);
        m_process->setCommand({device()->filePath("echo"), "-n $SSH_CLIENT", CommandLine::Raw});
        connect(m_process.get(), &Process::done, this, [this] {
            if (m_process->error() != QProcess::UnknownError) {
                reportFailure();
                return;
            }
            const QByteArrayList data = m_process->readAllRawStandardOutput().split(' ');
            if (data.size() != 3) {
                reportFailure();
                return;
            }
            QHostAddress hostAddress;
            if (!hostAddress.setAddress(QString::fromLatin1(data.first()))) {
                reportFailure();
                return;
            }
            *m_localServerAddress = hostAddress;
            reportStarted();
            m_process.release()->deleteLater();
        });
        m_process->start();
    }

    void stop() override
    {
        reportStopped();
    }

private:
    std::unique_ptr<Process> m_process = nullptr;
    QHostAddress *m_localServerAddress = nullptr;
};

QString MemcheckToolRunner::progressTitle() const
{
    return Tr::tr("Analyzing Memory");
}

void MemcheckToolRunner::start()
{
    m_runner.setLocalServerAddress(m_localServerAddress);
    ValgrindToolRunner::start();
}

void MemcheckToolRunner::stop()
{
    disconnect(&m_runner, &ValgrindProcess::internalError,
               this, &MemcheckToolRunner::internalParserError);
    ValgrindToolRunner::stop();
}

void MemcheckToolRunner::addToolArguments(CommandLine &cmd) const
{
    cmd << "--tool=memcheck" << "--gen-suppressions=all";

    if (m_settings.trackOrigins())
        cmd << "--track-origins=yes";

    if (m_settings.showReachable())
        cmd << "--show-reachable=yes";

    QString leakCheckValue;
    switch (m_settings.leakCheckOnFinish()) {
    case ValgrindSettings::LeakCheckOnFinishNo:
        leakCheckValue = "no";
        break;
    case ValgrindSettings::LeakCheckOnFinishYes:
        leakCheckValue = "full";
        break;
    case ValgrindSettings::LeakCheckOnFinishSummaryOnly:
    default:
        leakCheckValue = "summary";
        break;
    }
    cmd << "--leak-check=" + leakCheckValue;

    for (const FilePath &file : m_settings.suppressions())
        cmd << QString("--suppressions=%1").arg(file.path());

    cmd << QString("--num-callers=%1").arg(m_settings.numCallers());

    if (m_withGdb)
        cmd << "--vgdb=yes" << "--vgdb-error=0";

    cmd.addArgs(m_settings.memcheckArguments(), CommandLine::Raw);
}

const FilePaths MemcheckToolRunner::suppressionFiles() const
{
    return m_settings.suppressions();
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
        const QList<Stack> stacks = error.stacks();
        if (stacks.isEmpty())
            return defaultFrame;
        const Stack &stack = stacks[0];
        const QList<Frame> frames = stack.frames();
        if (frames.isEmpty())
            return defaultFrame;

        //find the first frame belonging to the project
        if (!projectFiles.isEmpty()) {
            for (const Frame &frame : frames) {
                if (frame.directory().isEmpty() || frame.fileName().isEmpty())
                    continue;

                //filepaths can contain "..", clean them:
                const QString f = QFileInfo(frame.filePath()).absoluteFilePath();
                if (projectFiles.contains(f))
                    return frame;
            }
        }

        //if no frame belonging to the project was found, return the first one that is not malloc/new
        for (const Frame &frame : frames) {
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
        for (Project *project : ProjectManager::projects()) {
            validFolders << project->projectDirectory().toString();
            const QList<Target *> targets = project->targets();
            for (const Target *target : targets) {
                const QList<DeployableFile> files = target->deploymentData().allFiles();
                for (const DeployableFile &file : files) {
                    if (file.isExecutable())
                        validFolders << file.remoteDirectory();
                }
                for (BuildConfiguration *config : target->buildConfigurations())
                    validFolders << config->buildDirectory().toString();
            }
        }

        const QList<Frame> frames = error.stacks().constFirst().frames();

        const int framesToLookAt = qMin(6, frames.size());

        bool inProject = false;
        for (int i = 0; i < framesToLookAt; ++i) {
            const Frame &frame = frames.at(i);
            for (const QString &folder : std::as_const(validFolders)) {
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

class MemcheckToolRunnerFactory final : public RunWorkerFactory
{
public:
    MemcheckToolRunnerFactory()
    {
        setProduct<MemcheckToolRunner>();
        addSupportedRunMode(MEMCHECK_RUN_MODE);
        addSupportedRunMode(MEMCHECK_WITH_GDB_RUN_MODE);
    }
};

class MemcheckToolPrivate : public QObject
{
public:
    MemcheckToolPrivate();
    ~MemcheckToolPrivate() override;

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
    ValgrindSettings *m_settings;
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

    std::unique_ptr<Parser> m_logParser;
    QString m_exitMsg;
    Perspective m_perspective{"Memcheck.Perspective", Tr::tr("Memcheck")};

    MemcheckToolRunnerFactory memcheckToolRunnerFactory;
};

static MemcheckToolPrivate *dd = nullptr;


class HeobDialog : public QDialog
{
public:
    HeobDialog(QWidget *parent);

    QString arguments() const;
    QString xmlName() const;
    bool attach() const;
    QString path() const;

    void keyPressEvent(QKeyEvent *e) override;

private:
    void updateProfile();
    void updateEnabled();
    void saveOptions();
    void newProfileDialog();
    void newProfile(const QString &name);
    void deleteProfileDialog();
    void deleteProfile();

private:
    QStringList m_profiles;
    QComboBox *m_profilesCombo = nullptr;
    QPushButton *m_profileDeleteButton = nullptr;
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

#ifdef Q_OS_WIN

class HeobData : public QObject
{
public:
    HeobData(MemcheckToolPrivate *mcTool, const QString &xmlPath, Kit *kit, bool attach);
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
    MemcheckToolPrivate *m_mcTool = nullptr;
    QString m_xmlPath;
    Kit *m_kit = nullptr;
    bool m_attach = false;
    RunControl *m_runControl = nullptr;
};
#endif

MemcheckToolPrivate::MemcheckToolPrivate()
{
    m_settings = &globalSettings();

    setObjectName("MemcheckTool");

    m_filterProjectAction = new QAction(Tr::tr("External Errors"), this);
    m_filterProjectAction->setToolTip(
        Tr::tr("Show issues originating outside currently opened projects."));
    m_filterProjectAction->setCheckable(true);

    m_suppressionSeparator = new QAction(Tr::tr("Suppressions"), this);
    m_suppressionSeparator->setSeparator(true);
    m_suppressionSeparator->setToolTip(
        Tr::tr("These suppression files were used in the last memory analyzer run."));

    QAction *a = new QAction(Tr::tr("Definite Memory Leaks"), this);
    initKindFilterAction(a, {Leak_DefinitelyLost, Leak_IndirectlyLost});
    m_errorFilterActions.append(a);

    a = new QAction(Tr::tr("Possible Memory Leaks"), this);
    initKindFilterAction(a, {Leak_PossiblyLost, Leak_StillReachable});
    m_errorFilterActions.append(a);

    a = new QAction(Tr::tr("Use of Uninitialized Memory"), this);
    initKindFilterAction(a, {InvalidRead, InvalidWrite, InvalidJump, Overlap,
                             InvalidMemPool, UninitCondition, UninitValue,
                             SyscallParam, ClientCheck});
    m_errorFilterActions.append(a);

    a = new QAction(Tr::tr("Invalid Calls to \"free()\""), this);
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
    m_errorView->setWindowTitle(Tr::tr("Memory Issues"));

    m_perspective.addWindow(m_errorView, Perspective::SplitVertical, nullptr);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
            this, &MemcheckToolPrivate::maybeActiveRunConfigurationChanged);

    //
    // The Control Widget.
    //

    m_startAction = Debugger::createStartAction();
    m_startWithGdbAction = Debugger::createStartAction();
    m_stopAction = Debugger::createStopAction();

    // Load external XML log file
    auto action = new QAction(this);
    action->setIcon(Icons::OPENFILE_TOOLBAR.icon());
    action->setToolTip(Tr::tr("Load External XML Log File"));
    connect(action, &QAction::triggered, this, &MemcheckToolPrivate::loadExternalXmlLogFile);
    m_loadExternalLogFile = action;

    // Go to previous leak.
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Icons::PREV_TOOLBAR.icon());
    action->setToolTip(Tr::tr("Go to previous leak."));
    connect(action, &QAction::triggered, m_errorView, &MemcheckErrorView::goBack);
    m_goBack = action;

    // Go to next leak.
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Icons::NEXT_TOOLBAR.icon());
    action->setToolTip(Tr::tr("Go to next leak."));
    connect(action, &QAction::triggered, m_errorView, &MemcheckErrorView::goNext);
    m_goNext = action;

    auto filterButton = new QToolButton;
    filterButton->setIcon(Icons::FILTER.icon());
    filterButton->setText(Tr::tr("Error Filter"));
    filterButton->setPopupMode(QToolButton::InstantPopup);
    filterButton->setProperty(StyleHelper::C_NO_ARROW, true);

    m_filterMenu = new QMenu(filterButton);
    for (QAction *filterAction : std::as_const(m_errorFilterActions))
        m_filterMenu->addAction(filterAction);
    m_filterMenu->addSeparator();
    m_filterMenu->addAction(m_filterProjectAction);
    m_filterMenu->addAction(m_suppressionSeparator);
    connect(m_filterMenu, &QMenu::triggered, this, &MemcheckToolPrivate::updateErrorFilter);
    filterButton->setMenu(m_filterMenu);

    ActionContainer *menu = ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);
    QString toolTip = Tr::tr("Valgrind Analyze Memory uses the Memcheck tool to find memory leaks.");

    if (!HostOsInfo::isWindowsHost()) {
        action = new QAction(this);
        action->setText(Tr::tr("Valgrind Memory Analyzer"));
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
        action->setText(Tr::tr("Valgrind Memory Analyzer with GDB"));
        action->setToolTip(Tr::tr("Valgrind Analyze Memory with GDB uses the "
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
        action = new QAction(Tr::tr("Heob"), this);
        Core::Command *cmd = Core::ActionManager::registerAction(action, "Memcheck.Local");
        cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Alt+H")));
        connect(action, &QAction::triggered, this, &MemcheckToolPrivate::heobAction);
        menu->addAction(cmd, Debugger::Constants::G_ANALYZER_TOOLS);
        connect(m_startAction, &QAction::changed, action, [action, this] {
            action->setEnabled(m_startAction->isEnabled());
        });
    }

    action = new QAction(this);
    action->setText(Tr::tr("Valgrind Memory Analyzer (External Application)"));
    action->setToolTip(toolTip);
    menu->addAction(ActionManager::registerAction(action, "Memcheck.Remote"),
                    Debugger::Constants::G_ANALYZER_REMOTE_TOOLS);
    QObject::connect(action, &QAction::triggered, this, [this, action] {
        RunConfiguration *runConfig = ProjectManager::startupRunConfiguration();
        if (!runConfig) {
            showCannotStartDialog(action->text());
            return;
        }
        StartRemoteDialog dlg;
        if (dlg.exec() != QDialog::Accepted)
            return;
        TaskHub::clearTasks(Debugger::Constants::ANALYZERTASK_ID);
        m_perspective.select();
        RunControl *rc = new RunControl(MEMCHECK_RUN_MODE);
        rc->copyDataFromRunConfiguration(runConfig);
        rc->createMainWorker();
        rc->setCommandLine(dlg.commandLine());
        rc->setWorkingDirectory(dlg.workingDirectory());
        ProjectExplorerPlugin::startRunControl(rc);
    });

    m_perspective.addToolBarAction(m_startAction);
    //toolbar.addAction(m_startWithGdbAction);
    m_perspective.addToolBarAction(m_stopAction);
    m_perspective.addToolBarAction(m_loadExternalLogFile);
    m_perspective.addToolBarAction(m_goBack);
    m_perspective.addToolBarAction(m_goNext);
    m_perspective.addToolBarWidget(filterButton);
    m_perspective.registerNextPrevShortcuts(m_goNext, m_goBack);

    updateFromSettings();
    maybeActiveRunConfigurationChanged();
}

MemcheckToolPrivate::~MemcheckToolPrivate()
{
    delete m_errorView;
}

void MemcheckToolPrivate::heobAction()
{
    ProcessRunData sr;
    Abi abi;
    bool hasLocalRc = false;
    Kit *kit = nullptr;
    if (Target *target = ProjectManager::startupTarget()) {
        if (RunConfiguration *rc = target->activeRunConfiguration()) {
            kit = target->kit();
            if (kit) {
                abi = ToolChainKitAspect::targetAbi(kit);
                sr = rc->runnable();
                const IDevice::ConstPtr device
                        = DeviceManager::deviceForPath(sr.command.executable());
                hasLocalRc = device && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
                if (!hasLocalRc)
                    hasLocalRc = DeviceTypeKitAspect::deviceTypeId(kit) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
            }
        }
    }
    if (!hasLocalRc) {
        const QString msg = Tr::tr("Heob: No local run configuration available.");
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        return;
    }
    if (abi.architecture() != Abi::X86Architecture
            || abi.os() != Abi::WindowsOS
            || abi.binaryFormat() != Abi::PEFormat
            || (abi.wordWidth() != 32 && abi.wordWidth() != 64)) {
        const QString msg = Tr::tr("Heob: No toolchain available.");
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        return;
    }

    FilePath executable = sr.command.executable();
    const QString workingDirectory = sr.workingDirectory.normalizedPathName().toString();
    const QString commandLineArguments = sr.command.arguments();
    const QStringList envStrings = sr.environment.toStringList();

    // target executable
    if (executable.isEmpty()) {
        const QString msg = Tr::tr("Heob: No executable set.");
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        return;
    }
    if (!executable.exists())
        executable = executable.withExecutableSuffix();
    if (!executable.exists()) {
        const QString msg = Tr::tr("Heob: Cannot find %1.").arg(executable.toUserOutput());
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        return;
    }

    // make executable a relative path if possible
    const QString wdSlashed = workingDirectory + '/';
    QString executablePath = executable.path();
    if (executablePath.startsWith(wdSlashed, Qt::CaseInsensitive)) {
        executablePath.remove(0, wdSlashed.size());
        executable = executable.withNewPath(executablePath);
    }

    // heob arguments
    HeobDialog dialog(Core::ICore::dialogParent());
    if (!dialog.exec())
        return;
    const QString heobArguments = dialog.arguments();

    // heob executable
    const QString heob = QString("heob%1.exe").arg(abi.wordWidth());
    const QString heobPath = dialog.path() + '/' + heob;
    if (!QFileInfo::exists(heobPath)) {
        QMessageBox::critical(
            Core::ICore::dialogParent(),
            Tr::tr("Heob"),
            Tr::tr("The %1 executables must be in the appropriate location.")
                .arg("<a href=\"https://github.com/ssbssa/heob/releases\">Heob</a>"));
        return;
    }

    // dwarfstack
    if (abi.osFlavor() == Abi::WindowsMSysFlavor) {
        const QString dwarfstack = QString("dwarfstack%1.dll").arg(abi.wordWidth());
        const QString dwarfstackPath = dialog.path() + '/' + dwarfstack;
        if (!QFileInfo::exists(dwarfstackPath)
            && CheckableMessageBox::information(
                   Core::ICore::dialogParent(),
                   Tr::tr("Heob"),
                   Tr::tr("Heob used with MinGW projects needs the %1 DLLs for proper "
                          "stacktrace resolution.")
                       .arg(
                           "<a "
                           "href=\"https://github.com/ssbssa/dwarfstack/releases\">Dwarfstack</a>"),
                   Key("HeobDwarfstackInfo"),
                   QMessageBox::Ok | QMessageBox::Cancel,
                   QMessageBox::Ok)
                   != QMessageBox::Ok)
            return;
    }

    // output xml file
    QDir wdDir(workingDirectory);
    const QString xmlPath = wdDir.absoluteFilePath(dialog.xmlName());
    QFile::remove(xmlPath);

    // full command line
    QString arguments = heob + heobArguments + " \"" + executable.path() + '\"';
    if (!commandLineArguments.isEmpty())
        arguments += ' ' + commandLineArguments;
    QByteArray argumentsCopy(reinterpret_cast<const char *>(arguments.utf16()), arguments.size() * 2 + 2);
    Q_UNUSED(argumentsCopy)

    // process environment
    QByteArray env;
    void *envPtr = nullptr;
    Q_UNUSED(envPtr)
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

#ifdef Q_OS_WIN
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
        const QString msg = Tr::tr("Heob: Cannot create %1 process (%2).")
                                .arg(heob)
                                .arg(qt_error_string(e));
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

void MemcheckToolPrivate::updateRunActions()
{
    if (m_toolBusy) {
        m_startAction->setEnabled(false);
        m_startAction->setToolTip(Tr::tr("A Valgrind Memcheck analysis is still in progress."));
        m_startWithGdbAction->setEnabled(false);
        m_startWithGdbAction->setToolTip(Tr::tr("A Valgrind Memcheck analysis is still in progress."));
        m_stopAction->setEnabled(true);
    } else {
        const auto canRun = ProjectExplorerPlugin::canRunStartupProject(MEMCHECK_RUN_MODE);
        m_startAction->setToolTip(canRun ? Tr::tr("Start a Valgrind Memcheck analysis.")
                                         : canRun.error());
        m_startAction->setEnabled(bool(canRun));
        const auto canRunGdb = ProjectExplorerPlugin::canRunStartupProject(
            MEMCHECK_WITH_GDB_RUN_MODE);
        m_startWithGdbAction->setToolTip(
            canRunGdb ? Tr::tr("Start a Valgrind Memcheck with GDB analysis.") : canRunGdb.error());
        m_startWithGdbAction->setEnabled(bool(canRunGdb));
        m_stopAction->setEnabled(false);
    }
}

void MemcheckToolPrivate::settingsDestroyed(QObject *settings)
{
    QTC_ASSERT(m_settings == settings, return);
    m_settings = &globalSettings();
}

void MemcheckToolPrivate::updateFromSettings()
{
    const QList<int> stored = m_settings->visibleErrorKinds();
    for (QAction *action : std::as_const(m_errorFilterActions)) {
        bool contained = true;
        const QList<QVariant> actions = action->data().toList();
        for (const QVariant &v : actions) {
            bool ok;
            int kind = v.toInt(&ok);
            if (ok && !stored.contains(kind)) {
                contained = false;
                break;
            }
        }
        action->setChecked(contained);
    }

    m_filterProjectAction->setChecked(!m_settings->filterExternalIssues());
    m_errorView->settingsChanged(m_settings);

    m_errorProxyModel.setAcceptedKinds(m_settings->visibleErrorKinds());
    connect(&m_settings->visibleErrorKinds, &BaseAspect::changed, &m_errorProxyModel, [this] {
        m_errorProxyModel.setAcceptedKinds(m_settings->visibleErrorKinds());
    });
    m_errorProxyModel.setFilterExternalIssues(m_settings->filterExternalIssues());
    connect(&m_settings->filterExternalIssues, &BaseAspect::changed, &m_errorProxyModel, [this] {
        m_errorProxyModel.setFilterExternalIssues(m_settings->filterExternalIssues());
    });
}

void MemcheckToolPrivate::maybeActiveRunConfigurationChanged()
{
    updateRunActions();

    ValgrindSettings *settings = nullptr;
    if (Project *project = ProjectManager::startupProject())
        if (Target *target = project->activeTarget())
            if (RunConfiguration *rc = target->activeRunConfiguration())
                settings = rc->currentSettings<ValgrindSettings>(ANALYZER_VALGRIND_SETTINGS);

    if (!settings) // fallback to global settings
        settings = &globalSettings();

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
    connect(m_settings, &ValgrindSettings::destroyed,
            this, &MemcheckToolPrivate::settingsDestroyed);

    updateFromSettings();
}

void MemcheckToolPrivate::setupRunner(MemcheckToolRunner *runTool)
{
    RunControl *runControl = runTool->runControl();
    m_errorModel.setRelevantFrameFinder(makeFrameFinder(transform(runControl->project()->files(Project::AllFiles),
                                                                  &FilePath::toString)));

    connect(runTool, &MemcheckToolRunner::parserError,
            this, &MemcheckToolPrivate::parserError);
    connect(runTool, &MemcheckToolRunner::internalParserError,
            this, &MemcheckToolPrivate::internalParserError);
    connect(runTool, &MemcheckToolRunner::stopped,
            this, &MemcheckToolPrivate::engineFinished);

    m_stopAction->disconnect();
    connect(m_stopAction, &QAction::triggered, runControl, &RunControl::initiateStop);

    m_toolBusy = true;
    updateRunActions();

    setBusyCursor(true);
    clearErrorView();
    m_loadExternalLogFile->setDisabled(true);

    const FilePath dir = runControl->project()->projectDirectory();
    const QString name = runControl->commandLine().executable().fileName();

    m_errorView->setDefaultSuppressionFile(dir.pathAppended(name + ".supp"));

    const FilePaths suppressionFiles = runTool->suppressionFiles();
    for (const FilePath &file : suppressionFiles) {
        QAction *action = m_filterMenu->addAction(file.fileName());
        action->setToolTip(file.toUserOutput());
        connect(action, &QAction::triggered, this, [file] {
            EditorManager::openEditorAt(file, 0);
        });
        m_suppressionActions.append(action);
    }
}

void MemcheckToolPrivate::loadShowXmlLogFile(const QString &filePath, const QString &exitMsg)
{
    clearErrorView();
    m_settings->filterExternalIssues.setValue(false);
    m_filterProjectAction->setChecked(true);
    m_perspective.select();
    Core::ModeManager::activateMode(Debugger::Constants::MODE_DEBUG);

    m_exitMsg = exitMsg;
    loadXmlLogFile(filePath);
}

void MemcheckToolPrivate::loadExternalXmlLogFile()
{
    const FilePath filePath = FileUtils::getOpenFilePath(
                nullptr,
                Tr::tr("Open Memcheck XML Log File"),
                {},
                Tr::tr("XML Files (*.xml);;All Files (*)"));
    if (filePath.isEmpty())
        return;

    m_exitMsg.clear();
    loadXmlLogFile(filePath.toString());
}

void MemcheckToolPrivate::loadXmlLogFile(const QString &filePath)
{
    QFile logFile(filePath);
    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString msg = Tr::tr("Memcheck: Failed to open file for reading: %1").arg(filePath);
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        if (!m_exitMsg.isEmpty())
            Debugger::showPermanentStatusMessage(m_exitMsg);
        return;
    }

    setBusyCursor(true);
    clearErrorView();
    m_loadExternalLogFile->setDisabled(true);

    if (!m_settings || m_settings != &globalSettings()) {
        m_settings = &globalSettings();
        m_errorView->settingsChanged(m_settings);
        updateFromSettings();
    }

    m_logParser.reset(new Parser);
    connect(m_logParser.get(), &Parser::error, this, &MemcheckToolPrivate::parserError);
    connect(m_logParser.get(), &Parser::done, this, [this](bool success, const QString &err) {
        if (!success)
            internalParserError(err);
        loadingExternalXmlLogFileFinished();
        m_logParser.release()->deleteLater();
    });

    m_logParser->setData(logFile.readAll());
    m_logParser->start();
}

void MemcheckToolPrivate::parserError(const Error &error)
{
    m_errorModel.addError(error);
}

void MemcheckToolPrivate::internalParserError(const QString &errorString)
{
    QString msg = Tr::tr("Memcheck: Error occurred parsing Valgrind output: %1").arg(errorString);
    TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
    TaskHub::requestPopup();
}

void MemcheckToolPrivate::clearErrorView()
{
    QTC_ASSERT(m_errorView, return);
    m_errorModel.clear();

    qDeleteAll(m_suppressionActions);
    m_suppressionActions.clear();
    //QTC_ASSERT(filterMenu()->actions().last() == m_suppressionSeparator, qt_noop());
}

void MemcheckToolPrivate::updateErrorFilter()
{
    QTC_ASSERT(m_errorView, return);
    QTC_ASSERT(m_settings, return);

    m_settings->filterExternalIssues.setValue(!m_filterProjectAction->isChecked());

    QList<int> errorKinds;
    for (QAction *a : std::as_const(m_errorFilterActions)) {
        if (!a->isChecked())
            continue;
        const QList<QVariant> actions = a->data().toList();
        for (const QVariant &v : actions) {
            bool ok;
            int kind = v.toInt(&ok);
            if (ok)
                errorKinds << kind;
        }
    }
    m_settings->visibleErrorKinds.setValue(errorKinds);
    m_settings->visibleErrorKinds.writeToSettingsImmediatly();
}

int MemcheckToolPrivate::updateUiAfterFinishedHelper()
{
    const int issuesFound = m_errorModel.rowCount();
    m_goBack->setEnabled(issuesFound > 1);
    m_goNext->setEnabled(issuesFound > 1);
    m_loadExternalLogFile->setEnabled(true);
    setBusyCursor(false);
    return issuesFound;
}

void MemcheckToolPrivate::engineFinished()
{
    m_toolBusy = false;
    updateRunActions();

    const int issuesFound = updateUiAfterFinishedHelper();
    Debugger::showPermanentStatusMessage(
        Tr::tr("Memory Analyzer Tool finished. %n issues were found.", nullptr, issuesFound));
}

void MemcheckToolPrivate::loadingExternalXmlLogFileFinished()
{
    const int issuesFound = updateUiAfterFinishedHelper();
    QString statusMessage = Tr::tr("Log file processed. %n issues were found.", nullptr, issuesFound);
    if (!m_exitMsg.isEmpty())
        statusMessage += ' ' + m_exitMsg;
    Debugger::showPermanentStatusMessage(statusMessage);
}

void MemcheckToolPrivate::setBusyCursor(bool busy)
{
    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_errorView->setCursor(cursor);
}

MemcheckToolRunner::MemcheckToolRunner(RunControl *runControl)
    : ValgrindToolRunner(runControl),
      m_withGdb(runControl->runMode() == MEMCHECK_WITH_GDB_RUN_MODE),
      m_localServerAddress(QHostAddress::LocalHost)
{
    setId("MemcheckToolRunner");
    connect(&m_runner, &ValgrindProcess::error, this, &MemcheckToolRunner::parserError);

    if (m_withGdb) {
        connect(&m_runner, &ValgrindProcess::valgrindStarted,
                this, &MemcheckToolRunner::startDebugger);
        connect(&m_runner, &ValgrindProcess::logMessageReceived,
                this, &MemcheckToolRunner::appendLog);
    } else {
        connect(&m_runner, &ValgrindProcess::internalError,
                this, &MemcheckToolRunner::internalParserError);
    }

    // We need a real address to connect to from the outside.
    if (device()->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        auto *dependentWorker = new LocalAddressFinder(runControl, &m_localServerAddress);
        addStartDependency(dependentWorker);
        addStopDependency(dependentWorker);
    }

    dd->setupRunner(this);
}

const char heobProfileC[] = "Heob/Profile";
const char heobProfileNameC[] = "Name";
const char heobXmlC[] = "Xml";
const char heobHandleExceptionC[] = "HandleException";
const char heobPageProtectionC[] = "PageProtection";
const char heobFreedProtectionC[] = "FreedProtection";
const char heobBreakpointC[] = "Breakpoint";
const char heobLeakDetailC[] = "LeakDetail";
const char heobLeakSizeC[] = "LeakSize";
const char heobLeakRecordingC[] = "LeakRecording";
const char heobAttachC[] = "Attach";
const char heobExtraArgsC[] = "ExtraArgs";
const char heobPathC[] = "Path";

HeobDialog::HeobDialog(QWidget *parent) :
    QDialog(parent)
{
    QtcSettings *settings = Core::ICore::settings();
    bool hasSelProfile = settings->contains(heobProfileC);
    const QString selProfile = hasSelProfile ? settings->value(heobProfileC).toString() : "Heob";
    m_profiles = settings->childGroups().filter(QRegularExpression("^Heob\\.Profile\\."));

    auto layout = new QVBoxLayout;
    // disable resizing
    layout->setSizeConstraint(QLayout::SetFixedSize);

    auto profilesLayout = new QHBoxLayout;
    m_profilesCombo = new QComboBox;
    for (const auto &profile : std::as_const(m_profiles))
        m_profilesCombo->addItem(settings->value(keyFromString(profile) + "/" + heobProfileNameC).toString());
    if (hasSelProfile) {
        int selIdx = m_profiles.indexOf(selProfile);
        if (selIdx >= 0)
            m_profilesCombo->setCurrentIndex(selIdx);
    }
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(1);
    m_profilesCombo->setSizePolicy(sizePolicy);
    connect(m_profilesCombo, &QComboBox::currentIndexChanged, this, &HeobDialog::updateProfile);
    profilesLayout->addWidget(m_profilesCombo);
    auto profileNewButton = new QPushButton(Tr::tr("New"));
    connect(profileNewButton, &QAbstractButton::clicked, this, &HeobDialog::newProfileDialog);
    profilesLayout->addWidget(profileNewButton);
    m_profileDeleteButton = new QPushButton(Tr::tr("Delete"));
    connect(m_profileDeleteButton, &QAbstractButton::clicked, this, &HeobDialog::deleteProfileDialog);
    profilesLayout->addWidget(m_profileDeleteButton);
    layout->addLayout(profilesLayout);

    auto xmlLayout = new QHBoxLayout;
    auto xmlLabel = new QLabel(Tr::tr("XML output file:"));
    xmlLayout->addWidget(xmlLabel);
    m_xmlEdit = new QLineEdit;
    xmlLayout->addWidget(m_xmlEdit);
    layout->addLayout(xmlLayout);

    auto handleExceptionLayout = new QHBoxLayout;
    auto handleExceptionLabel = new QLabel(Tr::tr("Handle exceptions:"));
    handleExceptionLayout->addWidget(handleExceptionLabel);
    m_handleExceptionCombo = new QComboBox;
    m_handleExceptionCombo->addItem(Tr::tr("Off"));
    m_handleExceptionCombo->addItem(Tr::tr("On"));
    m_handleExceptionCombo->addItem(Tr::tr("Only"));
    connect(m_handleExceptionCombo, &QComboBox::currentIndexChanged,
            this, &HeobDialog::updateEnabled);
    handleExceptionLayout->addWidget(m_handleExceptionCombo);
    layout->addLayout(handleExceptionLayout);

    auto pageProtectionLayout = new QHBoxLayout;
    auto pageProtectionLabel = new QLabel(Tr::tr("Page protection:"));
    pageProtectionLayout->addWidget(pageProtectionLabel);
    m_pageProtectionCombo = new QComboBox;
    m_pageProtectionCombo->addItem(Tr::tr("Off"));
    m_pageProtectionCombo->addItem(Tr::tr("After"));
    m_pageProtectionCombo->addItem(Tr::tr("Before"));
    connect(m_pageProtectionCombo, &QComboBox::currentIndexChanged,
            this, &HeobDialog::updateEnabled);
    pageProtectionLayout->addWidget(m_pageProtectionCombo);
    layout->addLayout(pageProtectionLayout);

    m_freedProtectionCheck = new QCheckBox(Tr::tr("Freed memory protection"));
    layout->addWidget(m_freedProtectionCheck);

    m_breakpointCheck = new QCheckBox(Tr::tr("Raise breakpoint exception on error"));
    layout->addWidget(m_breakpointCheck);

    auto leakDetailLayout = new QHBoxLayout;
    auto leakDetailLabel = new QLabel(Tr::tr("Leak details:"));
    leakDetailLayout->addWidget(leakDetailLabel);
    m_leakDetailCombo = new QComboBox;
    m_leakDetailCombo->addItem(Tr::tr("None"));
    m_leakDetailCombo->addItem(Tr::tr("Simple"));
    m_leakDetailCombo->addItem(Tr::tr("Detect Leak Types"));
    m_leakDetailCombo->addItem(Tr::tr("Detect Leak Types (Show Reachable)"));
    m_leakDetailCombo->addItem(Tr::tr("Fuzzy Detect Leak Types"));
    m_leakDetailCombo->addItem(Tr::tr("Fuzzy Detect Leak Types (Show Reachable)"));
    connect(m_leakDetailCombo, &QComboBox::currentIndexChanged, this, &HeobDialog::updateEnabled);
    leakDetailLayout->addWidget(m_leakDetailCombo);
    layout->addLayout(leakDetailLayout);

    auto leakSizeLayout = new QHBoxLayout;
    auto leakSizeLabel = new QLabel(Tr::tr("Minimum leak size:"));
    leakSizeLayout->addWidget(leakSizeLabel);
    m_leakSizeSpin = new QSpinBox;
    m_leakSizeSpin->setMinimum(0);
    m_leakSizeSpin->setMaximum(INT_MAX);
    m_leakSizeSpin->setSingleStep(1000);
    leakSizeLayout->addWidget(m_leakSizeSpin);
    layout->addLayout(leakSizeLayout);

    auto leakRecordingLayout = new QHBoxLayout;
    auto leakRecordingLabel = new QLabel(Tr::tr("Control leak recording:"));
    leakRecordingLayout->addWidget(leakRecordingLabel);
    m_leakRecordingCombo = new QComboBox;
    m_leakRecordingCombo->addItem(Tr::tr("Off"));
    m_leakRecordingCombo->addItem(Tr::tr("On (Start Disabled)"));
    m_leakRecordingCombo->addItem(Tr::tr("On (Start Enabled)"));
    leakRecordingLayout->addWidget(m_leakRecordingCombo);
    layout->addLayout(leakRecordingLayout);

    m_attachCheck = new QCheckBox(Tr::tr("Run with debugger"));
    layout->addWidget(m_attachCheck);

    auto extraArgsLayout = new QHBoxLayout;
    auto extraArgsLabel = new QLabel(Tr::tr("Extra arguments:"));
    extraArgsLayout->addWidget(extraArgsLabel);
    m_extraArgsEdit = new QLineEdit;
    extraArgsLayout->addWidget(m_extraArgsEdit);
    layout->addLayout(extraArgsLayout);

    auto pathLayout = new QHBoxLayout;
    auto pathLabel = new QLabel(Tr::tr("Heob path:"));
    pathLabel->setToolTip(Tr::tr("The location of heob32.exe and heob64.exe."));
    pathLayout->addWidget(pathLabel);
    m_pathChooser = new PathChooser;
    pathLayout->addWidget(m_pathChooser);
    layout->addLayout(pathLayout);

    auto saveLayout = new QHBoxLayout;
    saveLayout->addStretch(1);
    auto saveButton = new QToolButton;
    saveButton->setToolTip(Tr::tr("Save current settings as default."));
    saveButton->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    connect(saveButton, &QAbstractButton::clicked, this, &HeobDialog::saveOptions);
    saveLayout->addWidget(saveButton);
    layout->addLayout(saveLayout);

    auto okLayout = new QHBoxLayout;
    okLayout->addStretch(1);
    auto okButton = new QPushButton(Tr::tr("OK"));
    okButton->setDefault(true);
    connect(okButton, &QAbstractButton::clicked, this, &QDialog::accept);
    okLayout->addWidget(okButton);
    okLayout->addStretch(1);
    layout->addLayout(okLayout);

    setLayout(layout);

    updateProfile();

    if (!hasSelProfile) {
        settings->remove("heob");
        newProfile(Tr::tr("Default"));
    }
    m_profileDeleteButton->setEnabled(m_profilesCombo->count() > 1);

    setWindowTitle(Tr::tr("Heob"));
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
    return m_pathChooser->filePath().toString();
}

void HeobDialog::keyPressEvent(QKeyEvent *e)
{
    if (e->key() != Qt::Key_F1)
        return QDialog::keyPressEvent(e);

    reject();
    Core::HelpManager::showHelpUrl("qthelp://org.qt-project.qtcreator/doc/creator-heob.html");
}

void HeobDialog::updateProfile()
{
    QtcSettings *settings = Core::ICore::settings();
    const Key selProfile =
        keyFromString(m_profiles.empty() ? "heob" : m_profiles[m_profilesCombo->currentIndex()]);

    settings->beginGroup(selProfile);
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
    FilePath path = FilePath::fromSettings(settings->value(heobPathC));
    settings->endGroup();

    if (path.isEmpty()) {
        const QString heobPath = QStandardPaths::findExecutable("heob32.exe");
        if (!heobPath.isEmpty())
            path = FilePath::fromUserInput(heobPath);
    }

    m_xmlEdit->setText(xml);
    m_handleExceptionCombo->setCurrentIndex(handleException);
    m_pageProtectionCombo->setCurrentIndex(pageProtection);
    m_freedProtectionCheck->setChecked(freedProtection);
    m_breakpointCheck->setChecked(breakpoint);
    m_leakDetailCombo->setCurrentIndex(leakDetail);
    m_leakSizeSpin->setValue(leakSize);
    m_leakRecordingCombo->setCurrentIndex(leakRecording);
    m_attachCheck->setChecked(attach);
    m_extraArgsEdit->setText(extraArgs);
    m_pathChooser->setFilePath(path);
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
    QtcSettings *settings = Core::ICore::settings();
    const QString selProfile = m_profiles.at(m_profilesCombo->currentIndex());

    settings->setValue(heobProfileC, selProfile);

    settings->beginGroup(keyFromString(selProfile));
    settings->setValue(heobProfileNameC, m_profilesCombo->currentText());
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
    settings->setValue(heobPathC, m_pathChooser->filePath().toSettings());
    settings->endGroup();
}

void HeobDialog::newProfileDialog()
{
    QInputDialog *dialog = new QInputDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setWindowTitle(Tr::tr("New Heob Profile"));
    dialog->setLabelText(Tr::tr("Heob profile name:"));
    dialog->setTextValue(Tr::tr("%1 (copy)").arg(m_profilesCombo->currentText()));

    connect(dialog, &QInputDialog::textValueSelected, this, &HeobDialog::newProfile);
    dialog->open();
}

void HeobDialog::newProfile(const QString &name)
{
    int num = 1;
    while (m_profiles.indexOf(QString("Heob.Profile.%1").arg(num)) >= 0)
        num++;
    m_profiles.append(QString("Heob.Profile.%1").arg(num));
    m_profilesCombo->blockSignals(true);
    m_profilesCombo->addItem(name);
    m_profilesCombo->setCurrentIndex(m_profilesCombo->count() - 1);
    m_profilesCombo->blockSignals(false);
    saveOptions();
    m_profileDeleteButton->setEnabled(m_profilesCombo->count() > 1);
}

void HeobDialog::deleteProfileDialog()
{
    if (m_profilesCombo->count() < 2)
        return;

    QMessageBox *messageBox = new QMessageBox(QMessageBox::Warning,
                                              Tr::tr("Delete Heob Profile"),
                                              Tr::tr("Are you sure you want to delete this profile permanently?"),
                                              QMessageBox::Discard | QMessageBox::Cancel,
                                              this);

    // Change the text and role of the discard button
    auto deleteButton = static_cast<QPushButton*>(messageBox->button(QMessageBox::Discard));
    deleteButton->setText(Tr::tr("Delete"));
    messageBox->addButton(deleteButton, QMessageBox::AcceptRole);
    messageBox->setDefaultButton(deleteButton);

    connect(messageBox, &QDialog::accepted, this, &HeobDialog::deleteProfile);
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    messageBox->open();
}

void HeobDialog::deleteProfile()
{
    QtcSettings *settings = Core::ICore::settings();
    int index = m_profilesCombo->currentIndex();
    const QString profile = m_profiles.at(index);
    bool isDefault = settings->value(heobProfileC).toString() == profile;
    settings->remove(keyFromString(profile));
    m_profiles.removeAt(index);
    m_profilesCombo->removeItem(index);
    if (isDefault)
        settings->setValue(heobProfileC, m_profiles.at(m_profilesCombo->currentIndex()));
    m_profileDeleteButton->setEnabled(m_profilesCombo->count() > 1);
}

#ifdef Q_OS_WIN
static QString upperHexNum(unsigned num)
{
    return QString("%1").arg(num, 8, 16, QChar('0')).toUpper();
}

HeobData::HeobData(MemcheckToolPrivate *mcTool, const QString &xmlPath, Kit *kit, bool attach)
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
            m_runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
            m_runControl->setKit(m_kit);
            auto debugger = new DebuggerRunTool(m_runControl);
            debugger->setAttachPid(ProcessHandle(m_data[1]));
            debugger->setRunControlName(Tr::tr("Process %1").arg(m_data[1]));
            debugger->setStartMode(AttachToLocalProcess);
            debugger->setCloseMode(DetachAtClose);
            debugger->setContinueAfterAttach(true);
            debugger->setInferiorExecutable(FilePath::fromString(Utils::imageName(m_data[1])));

            connect(m_runControl, &RunControl::started, this, &HeobData::debugStarted);
            connect(m_runControl, &RunControl::stopped, this, &HeobData::debugStopped);
            debugger->startRunControl();
            return;
        }

        switch (m_data[0]) {
        case HEOB_OK:
            exitMsg = Tr::tr("Process finished with exit code %1 (0x%2).").arg(m_data[1]).arg(upperHexNum(m_data[1]));
            needErrorMsg = false;
            break;

        case HEOB_BAD_ARG:
            exitMsg = Tr::tr("Unknown argument: -%1").arg((char)m_data[1]);
            break;

        case HEOB_PROCESS_FAIL:
            exitMsg = Tr::tr("Cannot create target process.");
            if (m_data[1])
                exitMsg += " (" + qt_error_string(m_data[1]) + ')';
            break;

        case HEOB_WRONG_BITNESS:
            exitMsg = Tr::tr("Wrong bitness.");
            break;

        case HEOB_PROCESS_KILLED:
            exitMsg = Tr::tr("Process killed.");
            break;

        case HEOB_NO_CRT:
            exitMsg = Tr::tr("Only works with dynamically linked CRT.");
            break;

        case HEOB_EXCEPTION:
            exitMsg = Tr::tr("Process stopped with unhandled exception code 0x%1.").arg(upperHexNum(m_data[1]));
            needErrorMsg = false;
            break;

        case HEOB_OUT_OF_MEMORY:
            exitMsg = Tr::tr("Not enough memory to keep track of allocations.");
            break;

        case HEOB_UNEXPECTED_END:
            exitMsg = Tr::tr("Application stopped unexpectedly.");
            break;

        case HEOB_CONSOLE:
            exitMsg = Tr::tr("Extra console.");
            break;

        case HEOB_HELP:
        case HEOB_TRACE:
            deleteLater();
            return;

        default:
            exitMsg = Tr::tr("Unknown exit reason.");
            break;
        }
    } else {
        exitMsg = Tr::tr("Heob stopped unexpectedly.");
    }

    if (needErrorMsg) {
        const QString msg = Tr::tr("Heob: %1").arg(exitMsg);
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

    const QString msg = Tr::tr("Heob: Failure in process attach handshake (%1).").arg(qt_error_string(e));
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

MemcheckTool::MemcheckTool()
{
    dd = new MemcheckToolPrivate;
}

MemcheckTool::~MemcheckTool()
{
    delete dd;
}

} // Valgrind::Internal

#include "memchecktool.moc"
