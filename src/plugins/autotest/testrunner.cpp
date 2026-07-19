// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testrunner.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "autotesttr.h"
#include "testconfiguration.h"
#include "testoutputreader.h"
#include "testprojectsettings.h"
#include "testresultspane.h"
#include "testrunconfiguration.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/taskprogress.h>

#include <debugger/debuggerkitaspect.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <remote/remotelinux_constants.h> // soft dependency

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/outputformat.h>
#include <utils/qtcprocess.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QPointer>
#include <QPushButton>

using namespace Core;
using namespace Debugger;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Autotest::Internal {

static Q_LOGGING_CATEGORY(runnerLog, "qtc.autotest.testrunner", QtWarningMsg)

static TestRunner *s_instance = nullptr;

class RunConfigurationSelectionDialog final : public QDialog
{
public:
    explicit RunConfigurationSelectionDialog(const QString &buildTargetKey);
    QString displayName() const;
    QString executable() const;
    bool rememberChoice() const;
private:
    void populate();
    void updateLabels();
    QLabel *m_details;
    QLabel *m_executable;
    QLabel *m_arguments;
    QLabel *m_workingDir;
    QComboBox *m_rcCombo;
    QCheckBox *m_rememberCB;
    QDialogButtonBox *m_buttonBox;
};

TestRunner *TestRunner::instance()
{
    return s_instance;
}

TestRunner::TestRunner()
{
    s_instance = this;

    m_cancelTimer.setSingleShot(true);
    connect(&m_cancelTimer, &QTimer::timeout, this, [this] { cancelCurrent(Timeout); });
    connect(this, &TestRunner::requestStopTestRun, this, [this] { cancelCurrent(UserCanceled); });
    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            this, &TestRunner::onBuildQueueFinished);
}

TestRunner::~TestRunner()
{
    QTC_ASSERT(!m_currentRunControl, m_currentRunControl->forceStop());
    qDeleteAll(m_selectedTests);
    m_selectedTests.clear();
    s_instance = nullptr;
}

void TestRunner::runTest(TestRunMode mode, const ITestTreeItem *item)
{
    QTC_ASSERT(!isTestRunning(), return);
    ITestConfiguration *configuration = item->asConfiguration(mode);

    if (configuration)
        runTests(mode, {configuration});
}

static QString processInformation(const Process *proc)
{
    QTC_ASSERT(proc, return {});
    const CommandLine command = proc->commandLine();
    QString information("\nCommand line: " + command.executable().toUserOutput() + ' ' + command.arguments());
    QStringList important = { "PATH" };
    if (HostOsInfo::isLinuxHost())
        important.append("LD_LIBRARY_PATH");
    else if (HostOsInfo::isMacHost())
        important.append({ "DYLD_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH" });
    const Environment &environment = proc->environment();
    for (const QString &var : important)
        information.append('\n' + var + ": " + environment.value(var));
    return information;
}

static QString rcInfo(const ITestConfiguration * const config)
{
    if (config->testBase()->type() == ITestBase::Tool)
        return {};
    const TestConfiguration *current = static_cast<const TestConfiguration *>(config);
    QString info;
    if (current->isDeduced())
        info = Tr::tr("\nRun configuration: deduced from \"%1\"");
    else
        info = Tr::tr("\nRun configuration: \"%1\"");
    return info.arg(current->runConfigDisplayName());
}

static QString constructOmittedDetailsString(const QStringList &omitted)
{
    return Tr::tr("Omitted the following arguments specified on the run "
                  "configuration page for \"%1\":") + '\n' + omitted.join('\n');
}

static QString constructOmittedVariablesDetailsString(const EnvironmentItems &diff)
{
    auto removedVars = Utils::transform<QStringList>(diff, [](const EnvironmentItem &it) {
        return it.name;
    });
    return Tr::tr("Omitted the following environment variables for \"%1\":")
            + '\n' + removedVars.join('\n');
}

void TestRunner::cancelCurrent(TestRunner::CancelReason reason)
{
    if (reason == KitChanged)
        reportResult(ResultType::MessageWarn, Tr::tr("Current kit has changed. Canceling test run."));
    else if (reason == Timeout)
        reportResult(ResultType::MessageFatal, Tr::tr("Test case canceled due to timeout.\nMaybe raise the timeout?"));
    else if (reason == UserCanceled)
        reportResult(ResultType::MessageFatal, Tr::tr("Test run canceled by user."));
    if (m_currentRunControl) {
        m_runControlIndex = m_selectedTests.size();
        if (reason == UserCanceled && m_stopRequested)
            m_currentRunControl->forceStop();
        else
            m_currentRunControl->initiateStop();
        m_stopRequested = true;
    } else {
        m_taskTreeRunner.reset();
        onFinished();
    }
}

void TestRunner::runTests(TestRunMode mode, const QList<ITestConfiguration *> &selectedTests)
{
    QTC_ASSERT(!isTestRunning(), return);
    qDeleteAll(m_selectedTests);
    m_selectedTests = selectedTests;

    m_skipTargetsCheck = false;
    m_runMode = mode;
    if (mode != TestRunMode::RunAfterBuild
            && globalProjectExplorerSettings().buildBeforeDeploy() != BuildBeforeRunMode::Off
            && !globalProjectExplorerSettings().saveBeforeBuild()) {
        if (!ProjectExplorerPlugin::saveModifiedFiles())
            return;
    }

    emit testRunStarted();

    // clear old log and output pane
    TestResultsPane::instance()->clearContents();
    TestTreeModel::instance()->clearFailedMarks();

    if (m_selectedTests.isEmpty()) {
        reportResult(ResultType::MessageWarn, Tr::tr("No tests selected. Canceling test run."));
        onFinished();
        return;
    }

    Project *project = m_selectedTests.first()->project();
    if (!project) {
        reportResult(ResultType::MessageWarn,
            Tr::tr("Project is null. Canceling test run.\n"
                   "Only desktop kits are supported. Make sure the "
                   "currently active kit is a desktop kit."));
        onFinished();
        return;
    }

    m_targetConnect = connect(project, &Project::activeTargetChanged,
                              this, [this] { cancelCurrent(KitChanged); });

    if (globalProjectExplorerSettings().buildBeforeDeploy() == BuildBeforeRunMode::Off
            || mode == TestRunMode::DebugWithoutDeploy
            || mode == TestRunMode::RunWithoutDeploy || mode == TestRunMode::RunAfterBuild) {
        tryReconnectDevice();
        return;
    }

    Target *target = project->activeTarget();
    if (target && BuildConfigurationFactory::find(target)) {
        buildProject(project);
    } else {
        reportResult(ResultType::MessageFatal,
                     Tr::tr("Project is not configured. Canceling test run."));
        onFinished();
    }
}

static QString firstNonEmptyTestCaseTarget(const TestConfiguration *config)
{
    return Utils::findOrDefault(config->internalTargets(), [](const QString &internalTarget) {
        return !internalTarget.isEmpty();
    });
}

static RunConfiguration *getRunConfiguration(const QString &buildTargetKey)
{
    const BuildConfiguration * const buildConfig = activeBuildConfigForActiveProject();
    if (!buildConfig)
        return nullptr;

    RunConfiguration *runConfig = nullptr;
    const QList<RunConfiguration *> runConfigurations
            = Utils::filtered(buildConfig->runConfigurations(), [](const RunConfiguration *rc) {
        return !rc->runnable().command.isEmpty();
    });

    const ChoicePair oldChoice = cachedChoiceFor(buildTargetKey);
    if (!oldChoice.executable.isEmpty()) {
        runConfig = Utils::findOrDefault(runConfigurations,
                                         [&oldChoice](const RunConfiguration *rc) {
            return oldChoice.matches(rc);
        });
        if (runConfig)
            return runConfig;
    }

    if (runConfigurations.size() == 1)
        return runConfigurations.first();

    RunConfigurationSelectionDialog dialog(buildTargetKey);
    if (dialog.exec() == QDialog::Accepted) {
        const QString dName = dialog.displayName();
        if (dName.isEmpty())
            return nullptr;
        // run configuration has been selected - fill config based on this one..
        const FilePath exe = FilePath::fromString(dialog.executable());
        runConfig = Utils::findOr(runConfigurations, nullptr, [&dName, &exe](const RunConfiguration *rc) {
            if (rc->displayName() != dName)
                return false;
            return rc->runnable().command.executable() == exe;
        });
        if (runConfig && dialog.rememberChoice())
            cacheRunConfigChoice(buildTargetKey, ChoicePair(dName, exe));
    }
    return runConfig;
}

bool TestRunner::cancelAfterPreRunCheck()
{
    QList<ITestConfiguration *> toBeRemoved;
    bool projectChanged = false;
    for (ITestConfiguration *itc : std::as_const(m_selectedTests)) {
        if (itc->testBase()->type() == ITestBase::Tool) {
            if (itc->project() != ProjectManager::startupProject()) {
                projectChanged = true;
                toBeRemoved.append(itc);
            }
            continue;
        }
        TestConfiguration *config = static_cast<TestConfiguration *>(itc);
        config->completeTestInformation(isDebugMode() ? TestRunMode::Debug : TestRunMode::Run);
        if (!config->project()) {
            projectChanged = true;
            toBeRemoved.append(config);
        } else if (!config->hasExecutable()) {
            if (auto rc = getRunConfiguration(firstNonEmptyTestCaseTarget(config)))
                config->setOriginalRunConfiguration(rc);
            else
                toBeRemoved.append(config);
        }
    }
    for (ITestConfiguration *config : toBeRemoved)
        m_selectedTests.removeOne(config);
    qDeleteAll(toBeRemoved);
    if (m_selectedTests.isEmpty()) {
        const QString mssg = projectChanged
                ? Tr::tr("Startup project has changed. Canceling test run.")
                : Tr::tr("No test cases left for execution. Canceling test run.");
        reportResult(ResultType::MessageWarn, mssg);
        onFinished();
        return true;
    }
    return false;
}

int TestRunner::precheckTestConfigurations()
{
    const bool omitWarnings = testSettings().omitRunConfigWarn();
    int testCaseCount = 0;
    for (ITestConfiguration *itc : std::as_const(m_selectedTests)) {
        if (itc->testBase()->type() == ITestBase::Tool) {
            if (itc->project()) {
                testCaseCount += itc->testCaseCount();
            } else {
                reportResult(ResultType::MessageWarn,
                             Tr::tr("Project is null for \"%1\". Removing from test run.\n"
                                    "Check the test environment.").arg(itc->displayName()));
            }
            continue;
        }
        TestConfiguration *config = static_cast<TestConfiguration *>(itc);
        config->completeTestInformation(isDebugMode() ? TestRunMode::Debug : TestRunMode::Run);
        if (config->project()) {
            testCaseCount += config->testCaseCount();
            if (!omitWarnings && config->isDeduced()) {
                QString message = Tr::tr(
                            "Project's run configuration was deduced for \"%1\".\n"
                            "This might cause trouble during execution.\n"
                            "(deduced from \"%2\")");
                message = message.arg(config->displayName(), config->runConfigDisplayName());
                reportResult(ResultType::MessageWarn, message);
            }
        } else {
            reportResult(ResultType::MessageWarn,
                         Tr::tr("Project is null for \"%1\". Removing from test run.\n"
                                "Check the test environment.").arg(config->displayName()));
        }
    }
    return testCaseCount;
}

void TestRunner::onBuildSystemUpdated()
{
    BuildSystem *bs = activeBuildSystemForActiveProject();
    if (QTC_GUARD(bs))
        disconnect(bs, &BuildSystem::updated, this, &TestRunner::onBuildSystemUpdated);
    if (!m_skipTargetsCheck) {
        m_skipTargetsCheck = true;
        runOrDebugTests();
    }
}

void TestRunner::runTestsHelper()
{
    if (cancelAfterPreRunCheck())
        return;

    const int testCaseCount = precheckTestConfigurations();
    Q_UNUSED(testCaseCount) // TODO: may be useful for fine-grained progress reporting, when fixed

    struct TestStorage {
        std::unique_ptr<TestOutputReader> m_outputReader;
    };

    const ListIterator iterator(m_selectedTests);
    const Storage<TestStorage> storage;

    const auto onSetup = [this, iterator, storage](Process &process) {
        ITestConfiguration *config = *iterator;
        QTC_ASSERT(config, return SetupResult::StopWithError);
        if (!config->project())
            return SetupResult::StopWithSuccess;
        if (config->testExecutable().isEmpty()) {
            reportResult(ResultType::MessageFatal,
                         Tr::tr("Executable path is empty. (%1)").arg(config->displayName()));
            return SetupResult::StopWithSuccess;
        }
        TestStorage *testStorage = storage.activeStorage();
        QTC_ASSERT(testStorage, return SetupResult::StopWithError);
        testStorage->m_outputReader.reset(config->createOutputReader(&process));
        QTC_ASSERT(testStorage->m_outputReader, return SetupResult::StopWithError);
        connect(testStorage->m_outputReader.get(), &TestOutputReader::newResult,
                this, &TestRunner::testResultReady);
        connect(testStorage->m_outputReader.get(), &TestOutputReader::newOutputLineAvailable,
                TestResultsPane::instance(), &TestResultsPane::addOutputLine);

        CommandLine command{config->testExecutable()};
        if (config->testBase()->type() == ITestBase::Framework) {
            TestConfiguration *current = static_cast<TestConfiguration *>(config);
            QStringList omitted;
            command.addArgs(current->argumentsForTestRunner(&omitted).join(' '), CommandLine::Raw);
            if (!omitted.isEmpty()) {
                const QString &details = constructOmittedDetailsString(omitted);
                reportResult(ResultType::MessageWarn, details.arg(current->displayName()));
            }
        } else {
            TestToolConfiguration *current = static_cast<TestToolConfiguration *>(config);
            if (current->isInvalid()) {
                reportResult(ResultType::MessageWarn, current->errorMessage());
                return SetupResult::StopWithSuccess;
            }
            command.setArguments(current->commandLine().arguments());
        }
        process.setCommand(command);

        process.setWorkingDirectory(config->workingDirectory());
        const Environment &original = config->environment();
        Environment environment = config->filteredEnvironment(original);
        const EnvironmentItems removedVariables = Utils::filtered(
                    original.diff(environment), [](const EnvironmentItem &it) {
            return it.operation == EnvironmentItem::Unset;
        });
        if (!removedVariables.isEmpty()) {
            const QString &details = constructOmittedVariablesDetailsString(removedVariables)
                    .arg(config->displayName());
            reportResult(ResultType::MessageWarn, details);
        }
        process.setEnvironment(environment);

        if (testSettings().useTimeout()) {
            m_cancelTimer.setInterval(testSettings().timeout());
            m_cancelTimer.start();
        }

        qCInfo(runnerLog) << "Command:" << process.commandLine().executable();
        qCInfo(runnerLog) << "Arguments:" << process.commandLine().arguments();
        qCInfo(runnerLog) << "Working directory:" << process.workingDirectory();
        qCDebug(runnerLog) << "Environment:" << process.environment().toStringList();
        return SetupResult::Continue;
    };
    const auto onDone = [this, iterator, storage](const Process &process) {
        ITestConfiguration *config = *iterator;
        TestStorage *testStorage = storage.activeStorage();
        QTC_ASSERT(testStorage, return);
        if (process.result() == ProcessResult::StartFailed) {
            reportResult(ResultType::MessageFatal,
                Tr::tr("Failed to start test for project \"%1\".").arg(config->displayName())
                    + processInformation(&process) + rcInfo(config));
        }

        if (testStorage->m_outputReader)
            testStorage->m_outputReader->onDone(process.exitCode());

        if (process.exitStatus() == ProcessExitStatus::CrashExit) {
            if (testStorage->m_outputReader)
                testStorage->m_outputReader->reportCrash();
            reportResult(ResultType::MessageFatal,
                         Tr::tr("Test for project \"%1\" crashed.").arg(config->displayName())
                         + processInformation(&process) + rcInfo(config));
        } else if (testStorage->m_outputReader && !testStorage->m_outputReader->hadValidOutput()) {
            reportResult(ResultType::MessageFatal,
                         Tr::tr("Test for project \"%1\" did not produce any expected output.")
                         .arg(config->displayName()) + processInformation(&process)
                         + rcInfo(config));
        }
        if (testStorage->m_outputReader) {
            const int disabled = testStorage->m_outputReader->disabledTests();
            if (disabled > 0)
                emit hadDisabledTests(disabled);
            if (testStorage->m_outputReader->hasSummary())
                emit reportSummary(testStorage->m_outputReader->id(), testStorage->m_outputReader->summary());
            emit reportDuration(testStorage->m_outputReader->duration().value_or(
                process.processDuration().count()));

            testStorage->m_outputReader->resetCommandlineColor();
        }
    };

    const Group recipe = For (iterator) >> Do {
        finishAllAndSuccess,
        Group {
            storage,
            ProcessTask(onSetup, onDone)
        }
    };

    const auto onTaskTreeSetup = [this](QTaskTree &taskTree) {
        auto progress = new TaskProgress(&taskTree);
        progress->setDisplayName(Tr::tr("Running Tests"));
        progress->setAutoStopOnCancel(false);
        using namespace std::chrono_literals;
        progress->setHalfLifeTimePerTask(10s);
        connect(progress, &TaskProgress::canceled, this, [this, progress] {
            // Progress was a child of task tree which is going to be deleted directly.
            // Unwind properly.
            progress->setParent(nullptr);
            progress->deleteLater();
            cancelCurrent(UserCanceled);
        });
        if (testSettings().popupOnStart())
            popupResultsPane();
    };
    const auto onTaskTreeDone = [this] { onFinished(); };

    m_taskTreeRunner.start(recipe, onTaskTreeSetup, onTaskTreeDone);
}

static void processOutput(TestOutputReader *outputreader, const QString &msg, OutputFormat format)
{
    QByteArray message = msg.toUtf8();
    switch (format) {
    case OutputFormat::StdErrFormat:
    case OutputFormat::StdOutFormat:
    case OutputFormat::DebugFormat: {
        static const QByteArray gdbSpecialOut = "Qt: gdb: -nograb added to command-line options.\n"
                                                "\t Use the -dograb option to enforce grabbing.";
        if (message.startsWith(gdbSpecialOut))
            message = message.mid(gdbSpecialOut.length() + 1);
        // messages should have additional \n at the end (exception debug - we may get buffered)
        if (message.endsWith('\n'))
            message.chop(1);
        // iOS returns messages with \r\n
        message.replace("\r\n", "\n");

        for (const auto &line : message.split('\n')) {
            if (format == OutputFormat::StdOutFormat)
                outputreader->processStdOutput(line);
            else
                outputreader->processStdError(line);
        }
        break;
    }
    default:
        break; // channels we're not caring about
    }
}

RunControl *TestRunner::createRunControl(const Id mode, TestConfiguration *config)
{
    RunControl *runControl = new RunControl(mode);
    RunConfiguration *rc = config->originalRunConfiguration();
    if (!rc)
        rc = config->runConfiguration();
    runControl->copyDataFromRunConfiguration(rc);
    runControl->setSuppressApplicationOutput(true);

    QStringList omitted;
    CommandLine command{config->testExecutable()};
    const QStringList args = config->argumentsForTestRunner(&omitted);
    command.setArguments(ProcessArgs::joinArgs(args));
    if (!omitted.isEmpty()) {
        reportResult(ResultType::MessageWarn,
                     constructOmittedDetailsString(omitted).arg(config->displayName()));
    }

    finalizeRunControl(runControl, config, command);
    if (!runControl->createMainRecipe()) {
        reportResult(ResultType::MessageFatal,
                     Tr::tr("Failed to create recipe for running. (%1)").arg(config->displayName()));
        delete runControl;
        return nullptr;
    }
    return runControl;
}

RunControl *TestRunner::createRunControl(TestToolConfiguration *config)
{
    Project *project = config->project();
    if (!project)
        return nullptr;

    RunControl *runControl = new RunControl(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    if (auto bc = project->activeBuildConfiguration()) {
        runControl->setBuildConfiguration(bc);
    } else if (auto kit = project->activeKit()) {
        runControl->setKit(kit);
    } else {
        delete runControl;
        return nullptr;
    }

    runControl->setSuppressApplicationOutput(true);
    finalizeRunControl(runControl, config, config->commandLine());
    // no RunWorkerFactory matches empty runConfigId on non-desktop devices, so build
    // process recipe directly - Utils::Process dispatches via the FilePath device scheme
    runControl->setRunRecipe(runControl->processRecipe(runControl->processTask()));
    return runControl;
}

void TestRunner::finalizeRunControl(RunControl *runControl, ITestConfiguration *config,
                                    const CommandLine &command)
{
    runControl->setCommandLine(command);

    if (!config->workingDirectory().isEmpty())
        runControl->setWorkingDirectory(config->workingDirectory());
    const IDeviceConstPtr device = runControl->device();
    const bool isDesktop = (!device
                            || device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    const Environment original = isDesktop ? config->runnable().environment
                                           : command.executable().deviceEnvironment();
    const Environment environment = config->filteredEnvironment(original);
    const EnvironmentItems removedVariables = Utils::filtered(
                original.diff(environment), [](const EnvironmentItem &it) {
        return it.operation == EnvironmentItem::Unset;
    });
    if (!removedVariables.isEmpty()) {
        const QString &details = constructOmittedVariablesDetailsString(removedVariables)
                .arg(config->displayName());
        reportResult(ResultType::MessageWarn, details);
    }
    runControl->setEnvironment(environment);
}

RunControl *TestRunner::runControlFor(ITestConfiguration *itc)
{
    const QString name = itc->displayName();
    switch (itc->testBase()->type()) {
    case ITestBase::Framework: {
        auto *config = static_cast<TestConfiguration *>(itc);
        QString fatalMessage;
        if (!config->runConfiguration())
            fatalMessage = Tr::tr("Failed to get run configuration. (%1)").arg(name);
        else if (!config->originalRunConfiguration())
            fatalMessage = Tr::tr("Failed to get original run configuration. (%1)").arg(name);
        else if (config->testExecutable().isEmpty())
            fatalMessage = Tr::tr("Executable path is empty. (%1)").arg(name);

        if (!fatalMessage.isEmpty()) {
            reportResult(ResultType::MessageFatal, fatalMessage);
            return nullptr;
        }
        const Id mode = isDebugMode() ? Id{ProjectExplorer::Constants::DEBUG_RUN_MODE}
                                      : Id{ProjectExplorer::Constants::NORMAL_RUN_MODE};
        return createRunControl(mode, config);
    }
    case ITestBase::Tool: {
        if (isDebugMode()) {
            reportResult(ResultType::MessageWarn,
                         Tr::tr("Debugging is not supported for this test. (%1)").arg(name));
            return nullptr;
        }
        auto *toolConfig = static_cast<TestToolConfiguration *>(itc);
        if (toolConfig->isInvalid()) {
            reportResult(ResultType::MessageWarn, toolConfig->errorMessage());
            return nullptr;
        }
        if (toolConfig->testExecutable().isEmpty()) {
            reportResult(ResultType::MessageFatal,
                         Tr::tr("Executable path is empty. (%1)").arg(name));
            return nullptr;
        }
        RunControl *runControl = createRunControl(toolConfig);
        if (!runControl) {
            reportResult(ResultType::MessageFatal,
                         Tr::tr("Failed to create run configuration. (%1)").arg(name));
        }
        return runControl;
    }
    default:
        reportResult(ResultType::MessageFatal,
                     Tr::tr("Unexpected test configuration. (%1)")
                     .arg(QString::number(itc->testBase()->type())));
        return nullptr;
    }
}

void TestRunner::runTestsHelperViaRunControl()
{
    if (cancelAfterPreRunCheck())
        return;

    precheckTestConfigurations();

    if (testSettings().popupOnStart())
        popupResultsPane();

    m_outputWarningShown = false;
    m_runControlIndex = 0;
    m_stopRequested = false;
    runNextViaRunControl();
}

void TestRunner::runNextViaRunControl()
{
    if (m_runControlIndex >= m_selectedTests.size()) {
        onFinished();
        return;
    }

    ITestConfiguration *itc = m_selectedTests.at(m_runControlIndex);
    if (!itc || !itc->project()) {
        ++m_runControlIndex;
        QMetaObject::invokeMethod(this, &TestRunner::runNextViaRunControl, Qt::QueuedConnection);
        return;
    }

    const QString name = itc->displayName();
    RunControl *runControl = runControlFor(itc);

    if (!runControl) {
        ++m_runControlIndex;
        QMetaObject::invokeMethod(this, &TestRunner::runNextViaRunControl, Qt::QueuedConnection);
        return;
    }

    // Some debugger backends cannot forward the test output to our reader.
    bool useOutputProcessor = true;
    if (isDebugMode()) {
        QString message;
        if (Kit *kit = itc->project()->activeKit()) {
            if (Debugger::DebuggerKitAspect::engineType(kit) == Debugger::CdbEngineType) {
                message = Tr::tr("Unable to display test results when using CDB.");
                useOutputProcessor = false;
            } else if (auto devType = RunDeviceTypeKitAspect::deviceTypeId(kit);
                       devType != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE
                       && devType != Remote::Constants::GenericLinuxOsType) {
                message = Tr::tr("Unable to display test results when debugging on device.");
                useOutputProcessor = false;
            }
        }
        if (!m_outputWarningShown && !message.isEmpty()) { // only report once per test run
            m_outputWarningShown = true;
            reportResult(ResultType::MessageWarn, message);
        }
    }

    std::shared_ptr<TestOutputReader> outputReader;
    if (useOutputProcessor)
        outputReader.reset(itc->createOutputReader(nullptr));
    if (outputReader) {
        outputReader->setId(runControl->commandLine().executable().toUserOutput());
        connect(outputReader.get(), &TestOutputReader::newResult,
                this, &TestRunner::testResultReady);
        connect(outputReader.get(), &TestOutputReader::newOutputLineAvailable,
                TestResultsPane::instance(), &TestResultsPane::addOutputLine);
    }

    connect(runControl, &RunControl::appendMessage, this,
            [outputReader](const QString &msg, OutputFormat format) {
        if (outputReader)
            processOutput(outputReader.get(), msg, format);
    });

    m_currentRunControl = runControl;
    connect(runControl, &RunControl::stopped, this,
            [this, outputReader, name] {
        m_cancelTimer.stop();
        if (outputReader) {
            const int exitCode = m_currentRunControl
                    ? m_currentRunControl->lastExitCode().value_or(0) : 0;
            outputReader->onDone(exitCode);
            const int disabled = outputReader->disabledTests();
            if (disabled > 0)
                emit hadDisabledTests(disabled);
            if (outputReader->hasSummary())
                emit reportSummary(outputReader->id(), outputReader->summary());
            if (outputReader->duration())
                emit reportDuration(*outputReader->duration());
            if (!outputReader->hadValidOutput()) {
                reportResult(ResultType::MessageFatal,
                             Tr::tr("Test for project \"%1\" did not produce any expected output.")
                                 .arg(name));
            }
        }

        m_currentRunControl.clear();
        ++m_runControlIndex;
        QMetaObject::invokeMethod(this, &TestRunner::runNextViaRunControl, Qt::QueuedConnection);
    });

    // Do not time out while debugging - the user drives the session.
    if (!isDebugMode() && testSettings().useTimeout()) {
        m_cancelTimer.setInterval(testSettings().timeout());
        m_cancelTimer.start();
    }
    runControl->start();
}

static bool executablesEmpty()
{
    const BuildConfiguration * const buildConfig = activeBuildConfigForActiveProject();
    QTC_ASSERT(buildConfig, return false);
    const QList<RunConfiguration *> configs = buildConfig->runConfigurations();
    QTC_ASSERT(!configs.isEmpty(), return false);
    if (auto execAspect = configs.first()->aspect<ExecutableAspect>())
        return execAspect->executable().isEmpty();
    return false;
}

void TestRunner::runOrDebugTests()
{
    if (!m_skipTargetsCheck) {
        if (executablesEmpty()) {
            m_skipTargetsCheck = true;
            BuildSystem *bs = activeBuildSystemForActiveProject();
            QTimer::singleShot(5000, this, [this, bs = QPointer<BuildSystem>(bs)] {
                if (bs) {
                    disconnect(bs, &BuildSystem::updated,
                               this, &TestRunner::onBuildSystemUpdated);
                }
                runOrDebugTests();
            });
            connect(bs, &BuildSystem::updated, this, &TestRunner::onBuildSystemUpdated);
            return;
        }
    }

    switch (m_runMode) {
    case TestRunMode::Run:
    case TestRunMode::RunWithoutDeploy:
    case TestRunMode::RunAfterBuild: {
        Project *project = ProjectManager::startupProject();
        const Kit *kit = project ? project->activeKit() : nullptr;
        const Id deviceTypeId = RunDeviceTypeKitAspect::deviceTypeId(kit);
        if (kit && deviceTypeId != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
            runTestsHelperViaRunControl();
        else
            runTestsHelper();
        return;
    }
    case TestRunMode::Debug:
    case TestRunMode::DebugWithoutDeploy:
        runTestsHelperViaRunControl();
        return;
    default:
        break;
    }
    QTC_ASSERT(false, qDebug() << "Unexpected run mode" << int(m_runMode));  // unexpected run mode
    onFinished();
}

void TestRunner::buildProject(Project *project)
{
    BuildManager *buildManager = BuildManager::instance();
    m_buildConnect = connect(this, &TestRunner::requestStopTestRun,
                             buildManager, &BuildManager::cancel);
    connect(buildManager, &BuildManager::buildQueueFinished, this, &TestRunner::buildFinished);
    BuildManager::buildProjectWithDependencies(project);
    if (!BuildManager::isBuilding())
        buildFinished(false);
}

void TestRunner::buildFinished(bool success)
{
    BuildManager *buildManager = BuildManager::instance();
    disconnect(buildManager, &BuildManager::buildQueueFinished, this, &TestRunner::buildFinished);

    if (!success) {
        disconnect(m_buildConnect);
        reportResult(ResultType::MessageFatal, Tr::tr("Build failed. Canceling test run."));
        onFinished();
        return;
    }
    connect(buildManager, &BuildManager::buildQueueFinished, this, &TestRunner::deployFinished);
    if (globalProjectExplorerSettings().deployBeforeRun()) {
        Project *project = m_selectedTests.isEmpty() ? nullptr : m_selectedTests.first()->project();
        if (project) {
            DeployConfiguration *deployConfig = project->activeDeployConfiguration();
            if (!deployConfig || !deployConfig->stepList() || deployConfig->stepList()->isEmpty()) {
                deployFinished(true);
                return;
            }
            BuildManager::deployProjects({project});
        }
        if (!BuildManager::isDeploying())
            deployFinished(false);
    } else {
        deployFinished(true);
    }
}

void TestRunner::deployFinished(bool success)
{
    disconnect(m_buildConnect);
    BuildManager *buildManager = BuildManager::instance();
    disconnect(buildManager, &BuildManager::buildQueueFinished, this, &TestRunner::deployFinished);

    if (success) {
        tryReconnectDevice();
        return;
    }
    reportResult(ResultType::MessageFatal, Tr::tr("Deploy failed. Canceling test run."));
    onFinished();
}

void TestRunner::tryReconnectDevice()
{
    Project *project = m_selectedTests.isEmpty() ? nullptr : m_selectedTests.first()->project();
    const Kit *kit = project ? project->activeKit() : nullptr;
    const Id deviceTypeId = RunDeviceTypeKitAspect::deviceTypeId(kit);
    if (!kit || deviceTypeId == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        runOrDebugTests();
        return;
    }
    const IDeviceConstPtr device = RunDeviceKitAspect::device(kit);
    if (!device) {
        runOrDebugTests();
        return;
    }
    const auto state = device->deviceState();
    if (state == IDevice::DeviceReadyToUse || state == IDevice::DeviceConnected) {
        runOrDebugTests();
        return;
    }
    qCDebug(runnerLog) << "Device not accessible - trying to reconnect"
                       << device->displayName();
    device->tryToConnect(Continuation<>(this, [this](const Result<> &result) {
        if (result == ResultOk) {
            QMetaObject::invokeMethod(this, &TestRunner::runOrDebugTests, Qt::QueuedConnection);
        } else {
            reportResult(ResultType::MessageFatal,
                         Tr::tr("Device connection failed. Canceling test run.")
                         .append(' ').append(result.error()));
            onFinished();
        }
    }));
}

static RunAfterBuildMode runAfterBuild()
{
    Project *project = ProjectManager::startupProject();
    if (!project)
        return RunAfterBuildMode::None;

    if (!project->namedSettings(Constants::SK_USE_GLOBAL).isValid())
        return testSettings().runAfterBuildMode();

    TestProjectSettings *projectSettings = Internal::testProjectSettings(project);
    return projectSettings->useGlobalSettings() ? testSettings().runAfterBuildMode()
                                                : projectSettings->runAfterBuild();
}

void TestRunner::onBuildQueueFinished(bool success)
{
    if (isTestRunning() || !m_selectedTests.isEmpty())  // paranoia!
        return;

    if (!success || m_runMode != TestRunMode::None)
        return;

    RunAfterBuildMode mode = runAfterBuild();
    if (mode == RunAfterBuildMode::None)
        return;

    auto testTreeModel = TestTreeModel::instance();
    if (!testTreeModel->hasTests(false))
        return;

    const QList<ITestConfiguration *> tests = mode == RunAfterBuildMode::All
            ? testTreeModel->getAllTestCases(m_runMode) : testTreeModel->getSelectedTests(m_runMode);
    runTests(TestRunMode::RunAfterBuild, tests);
}

void TestRunner::onFinished()
{
    m_taskTreeRunner.reset();
    disconnect(m_targetConnect);
    qDeleteAll(m_selectedTests);
    m_selectedTests.clear();
    m_cancelTimer.stop();
    m_runMode = TestRunMode::None;
    emit testRunFinished();
    QTC_ASSERT(!m_currentRunControl, m_currentRunControl->forceStop());
}

void TestRunner::reportResult(ResultType type, const QString &description)
{
    TestResult result({}, {});
    result.setResult(type);
    result.setDescription(description);
    emit testResultReady(result);
}

/*************************************************************************************************/

RunConfigurationSelectionDialog::RunConfigurationSelectionDialog(const QString &buildTargetKey)
    : QDialog(ICore::dialogParent())
{
    setWindowTitle(Tr::tr("Select Run Configuration"));

    QString details = Tr::tr("Could not determine which run configuration to choose for running tests");
    if (!buildTargetKey.isEmpty())
        details.append(QString(" (%1)").arg(buildTargetKey));
    m_details = new QLabel(details, this);
    m_rcCombo = new QComboBox(this);
    m_rememberCB = new QCheckBox(Tr::tr("Remember choice. Cached choices can be reset by switching "
                                        "projects or using the option to clear the cache."), this);
    m_executable = new QLabel(this);
    m_arguments = new QLabel(this);
    m_workingDir = new QLabel(this);
    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    m_buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    auto formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(m_details);
    formLayout->addRow(Tr::tr("Run Configuration:"), m_rcCombo);
    formLayout->addRow(m_rememberCB);
    formLayout->addRow(Layouting::createHr(this));
    formLayout->addRow(Tr::tr("Executable:"), m_executable);
    formLayout->addRow(Tr::tr("Arguments:"), m_arguments);
    formLayout->addRow(Tr::tr("Working Directory:"), m_workingDir);
    // TODO Device support
    auto vboxLayout = new QVBoxLayout(this);
    vboxLayout->addLayout(formLayout);
    vboxLayout->addStretch();
    vboxLayout->addWidget(Layouting::createHr(this));
    vboxLayout->addWidget(m_buttonBox);

    connect(m_rcCombo, &QComboBox::currentTextChanged,
            this, &RunConfigurationSelectionDialog::updateLabels);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    populate();
}

QString RunConfigurationSelectionDialog::displayName() const
{
    return m_rcCombo ? m_rcCombo->currentText() : QString();
}

QString RunConfigurationSelectionDialog::executable() const
{
    return m_executable ? m_executable->text() : QString();
}

bool RunConfigurationSelectionDialog::rememberChoice() const
{
    return m_rememberCB ? m_rememberCB->isChecked() : false;
}

void RunConfigurationSelectionDialog::populate()
{
    m_rcCombo->addItem({}, QStringList{{}, {}, {}}); // empty default

    if (auto buildConfig = activeBuildConfigForActiveProject()) {
        for (RunConfiguration *rc : buildConfig->runConfigurations()) {
            auto runnable = rc->runnable();
            const QStringList rcDetails
                = {runnable.command.executable().toUserOutput(),
                   runnable.command.arguments(),
                   runnable.workingDirectory.toUserOutput()};
            m_rcCombo->addItem(rc->displayName(), rcDetails);
        }
    }
}

void RunConfigurationSelectionDialog::updateLabels()
{
    int i = m_rcCombo->currentIndex();
    const QStringList values = m_rcCombo->itemData(i).toStringList();
    QTC_ASSERT(values.size() == 3, return);
    m_executable->setText(values.at(0));
    m_arguments->setText(values.at(1));
    m_workingDir->setText(values.at(2));
}

} // namespace Internal::Autotest
