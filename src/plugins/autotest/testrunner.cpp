// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testrunner.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "autotesttr.h"
#include "testoutputreader.h"
#include "testprojectsettings.h"
#include "testresultspane.h"
#include "testrunconfiguration.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/taskprogress.h>

#include <debugger/debuggerkitaspect.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/outputformat.h>
#include <utils/process.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QPointer>
#include <QPushButton>

using namespace Core;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace Autotest {
namespace Internal {

static Q_LOGGING_CATEGORY(runnerLog, "qtc.autotest.testrunner", QtWarningMsg)

static TestRunner *s_instance = nullptr;

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
    m_taskTree.reset();
    onFinished();
}

void TestRunner::runTests(TestRunMode mode, const QList<ITestConfiguration *> &selectedTests)
{
    QTC_ASSERT(!isTestRunning(), return);
    qDeleteAll(m_selectedTests);
    m_selectedTests = selectedTests;

    m_skipTargetsCheck = false;
    m_runMode = mode;
    const ProjectExplorerSettings projectExplorerSettings
            = ProjectExplorerPlugin::projectExplorerSettings();
    if (mode != TestRunMode::RunAfterBuild
            && projectExplorerSettings.buildBeforeDeploy != BuildBeforeRunMode::Off
            && !projectExplorerSettings.saveBeforeBuild) {
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

    if (projectExplorerSettings.buildBeforeDeploy == BuildBeforeRunMode::Off
            || mode == TestRunMode::DebugWithoutDeploy
            || mode == TestRunMode::RunWithoutDeploy || mode == TestRunMode::RunAfterBuild) {
        runOrDebugTests();
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
    const Project *project = ProjectManager::startupProject();
    if (!project)
        return nullptr;
    const Target *target = project->activeTarget();
    if (!target)
        return nullptr;

    RunConfiguration *runConfig = nullptr;
    const QList<RunConfiguration *> runConfigurations
            = Utils::filtered(target->runConfigurations(), [](const RunConfiguration *rc) {
        return !rc->runnable().command.isEmpty();
    });

    const ChoicePair oldChoice = AutotestPlugin::cachedChoiceFor(buildTargetKey);
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

    RunConfigurationSelectionDialog dialog(buildTargetKey, ICore::dialogParent());
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
            AutotestPlugin::cacheRunConfigChoice(buildTargetKey, ChoicePair(dName, exe));
    }
    return runConfig;
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
        config->completeTestInformation(TestRunMode::Run);
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
    Target *target = ProjectManager::startupTarget();
    if (QTC_GUARD(target))
        disconnect(target, &Target::buildSystemUpdated, this, &TestRunner::onBuildSystemUpdated);
    if (!m_skipTargetsCheck) {
        m_skipTargetsCheck = true;
        runOrDebugTests();
    }
}

void TestRunner::runTestsHelper()
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
        config->completeTestInformation(TestRunMode::Run);
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
    toBeRemoved.clear();
    if (m_selectedTests.isEmpty()) {
        QString mssg = projectChanged ? Tr::tr("Startup project has changed. Canceling test run.")
                                      : Tr::tr("No test cases left for execution. Canceling test run.");

        reportResult(ResultType::MessageWarn, mssg);
        onFinished();
        return;
    }

    const int testCaseCount = precheckTestConfigurations();
    Q_UNUSED(testCaseCount) // TODO: may be useful for fine-grained progress reporting, when fixed

    struct TestStorage {
        std::unique_ptr<TestOutputReader> m_outputReader;
    };

    QList<GroupItem> tasks{finishAllAndDone};

    for (ITestConfiguration *config : m_selectedTests) {
        QTC_ASSERT(config, continue);
        const TreeStorage<TestStorage> storage;

        const auto onSetup = [this, config] {
            if (!config->project())
                return SetupResult::StopWithDone;
            if (config->testExecutable().isEmpty()) {
                reportResult(ResultType::MessageFatal,
                             Tr::tr("Executable path is empty. (%1)").arg(config->displayName()));
                return SetupResult::StopWithDone;
            }
            return SetupResult::Continue;
        };
        const auto onProcessSetup = [this, config, storage](Process &process) {
            TestStorage *testStorage = storage.activeStorage();
            QTC_ASSERT(testStorage, return);
            testStorage->m_outputReader.reset(config->createOutputReader(&process));
            QTC_ASSERT(testStorage->m_outputReader, return);
            connect(testStorage->m_outputReader.get(), &TestOutputReader::newResult,
                    this, &TestRunner::testResultReady);
            connect(testStorage->m_outputReader.get(), &TestOutputReader::newOutputLineAvailable,
                    TestResultsPane::instance(), &TestResultsPane::addOutputLine);

            CommandLine command{config->testExecutable(), {}};
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

            m_cancelTimer.setInterval(testSettings().timeout());
            m_cancelTimer.start();

            qCInfo(runnerLog) << "Command:" << process.commandLine().executable();
            qCInfo(runnerLog) << "Arguments:" << process.commandLine().arguments();
            qCInfo(runnerLog) << "Working directory:" << process.workingDirectory();
            qCDebug(runnerLog) << "Environment:" << process.environment().toStringList();
        };
        const auto onProcessDone = [this, config, storage](const Process &process) {
            TestStorage *testStorage = storage.activeStorage();
            QTC_ASSERT(testStorage, return);
            if (process.result() == ProcessResult::StartFailed) {
                reportResult(ResultType::MessageFatal,
                    Tr::tr("Failed to start test for project \"%1\".").arg(config->displayName())
                        + processInformation(&process) + rcInfo(config));
            }

            if (testStorage->m_outputReader)
                testStorage->m_outputReader->onDone(process.exitCode());

            if (process.exitStatus() == QProcess::CrashExit) {
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

                testStorage->m_outputReader->resetCommandlineColor();
            }
        };
        const Group group {
            finishAllAndDone,
            Tasking::Storage(storage),
            onGroupSetup(onSetup),
            ProcessTask(onProcessSetup, onProcessDone, onProcessDone)
        };
        tasks.append(group);
    }

    m_taskTree.reset(new TaskTree(tasks));
    connect(m_taskTree.get(), &TaskTree::done, this, &TestRunner::onFinished);
    connect(m_taskTree.get(), &TaskTree::errorOccurred, this, &TestRunner::onFinished);

    auto progress = new TaskProgress(m_taskTree.get());
    progress->setDisplayName(Tr::tr("Running Tests"));
    progress->setAutoStopOnCancel(false);
    progress->setHalfLifeTimePerTask(10000); // 10 seconds
    connect(progress, &TaskProgress::canceled, this, [this, progress] {
        // progress was a child of task tree which is going to be deleted directly. Unwind properly.
        progress->setParent(nullptr);
        progress->deleteLater();
        cancelCurrent(UserCanceled);
    });

    if (testSettings().popupOnStart())
        AutotestPlugin::popupResultsPane();

    m_taskTree->start();
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
        message.chop(1); // all messages have an additional \n at the end

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

void TestRunner::debugTests()
{
    // TODO improve to support more than one test configuration
    QTC_ASSERT(m_selectedTests.size() == 1, onFinished(); return);

    ITestConfiguration *itc = m_selectedTests.first();
    QTC_ASSERT(itc->testBase()->type() == ITestBase::Framework, onFinished(); return);

    TestConfiguration *config = static_cast<TestConfiguration *>(itc);
    config->completeTestInformation(TestRunMode::Debug);
    if (!config->project()) {
        reportResult(ResultType::MessageWarn,
                     Tr::tr("Startup project has changed. Canceling test run."));
        onFinished();
        return;
    }
    if (!config->hasExecutable()) {
        if (auto *rc = getRunConfiguration(firstNonEmptyTestCaseTarget(config)))
            config->completeTestInformation(rc, TestRunMode::Debug);
    }

    if (!config->runConfiguration()) {
        reportResult(ResultType::MessageFatal, Tr::tr("Failed to get run configuration."));
        onFinished();
        return;
    }

    const FilePath &commandFilePath = config->executableFilePath();
    if (commandFilePath.isEmpty()) {
        reportResult(ResultType::MessageFatal, Tr::tr("Could not find command \"%1\". (%2)")
                     .arg(config->executableFilePath().toString(), config->displayName()));
        onFinished();
        return;
    }

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->copyDataFromRunConfiguration(config->runConfiguration());

    QStringList omitted;
    ProcessRunData inferior = config->runnable();
    inferior.command.setExecutable(commandFilePath);

    const QStringList args = config->argumentsForTestRunner(&omitted);
    inferior.command.setArguments(ProcessArgs::joinArgs(args));
    if (!omitted.isEmpty()) {
        const QString &details = constructOmittedDetailsString(omitted);
        reportResult(ResultType::MessageWarn, details.arg(config->displayName()));
    }
    Environment original(inferior.environment);
    inferior.environment = config->filteredEnvironment(original);
    const EnvironmentItems removedVariables = Utils::filtered(
        original.diff(inferior.environment), [](const EnvironmentItem &it) {
            return it.operation == EnvironmentItem::Unset;
        });
    if (!removedVariables.isEmpty()) {
        const QString &details = constructOmittedVariablesDetailsString(removedVariables)
                .arg(config->displayName());
        reportResult(ResultType::MessageWarn, details);
    }
    auto debugger = new Debugger::DebuggerRunTool(runControl);
    debugger->setInferior(inferior);
    debugger->setRunControlName(config->displayName());

    bool useOutputProcessor = true;
    if (Target *targ = config->project()->activeTarget()) {
        if (Debugger::DebuggerKitAspect::engineType(targ->kit()) == Debugger::CdbEngineType) {
            reportResult(ResultType::MessageWarn,
                         Tr::tr("Unable to display test results when using CDB."));
            useOutputProcessor = false;
        }
    }

    if (useOutputProcessor) {
        TestOutputReader *outputreader = config->createOutputReader(nullptr);
        connect(outputreader, &TestOutputReader::newResult, this, &TestRunner::testResultReady);
        outputreader->setId(inferior.command.executable().toString());
        connect(outputreader, &TestOutputReader::newOutputLineAvailable,
                TestResultsPane::instance(), &TestResultsPane::addOutputLine);
        connect(runControl, &RunControl::appendMessage,
                this, [outputreader](const QString &msg, OutputFormat format) {
            processOutput(outputreader, msg, format);
        });
        connect(runControl, &RunControl::stopped, outputreader, &QObject::deleteLater);
    }

    m_stopDebugConnect = connect(this, &TestRunner::requestStopTestRun,
                                 runControl, &RunControl::initiateStop);

    connect(runControl, &RunControl::stopped, this, &TestRunner::onFinished);
    ProjectExplorerPlugin::startRunControl(runControl);
    if (useOutputProcessor && testSettings().popupOnStart())
        AutotestPlugin::popupResultsPane();
}

static bool executablesEmpty()
{
    Target *target = ProjectManager::startupTarget();
    const QList<RunConfiguration *> configs = target->runConfigurations();
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
            Target *target = ProjectManager::startupTarget();
            QTimer::singleShot(5000, this, [this, target = QPointer<Target>(target)] {
                if (target) {
                    disconnect(target, &Target::buildSystemUpdated,
                               this, &TestRunner::onBuildSystemUpdated);
                }
                runOrDebugTests();
            });
            connect(target, &Target::buildSystemUpdated, this, &TestRunner::onBuildSystemUpdated);
            return;
        }
    }

    switch (m_runMode) {
    case TestRunMode::Run:
    case TestRunMode::RunWithoutDeploy:
    case TestRunMode::RunAfterBuild:
        runTestsHelper();
        return;
    case TestRunMode::Debug:
    case TestRunMode::DebugWithoutDeploy:
        debugTests();
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
    disconnect(m_buildConnect);
    BuildManager *buildManager = BuildManager::instance();
    disconnect(buildManager, &BuildManager::buildQueueFinished, this, &TestRunner::buildFinished);

    if (success) {
        runOrDebugTests();
        return;
    }
    reportResult(ResultType::MessageFatal, Tr::tr("Build failed. Canceling test run."));
    onFinished();
}

static RunAfterBuildMode runAfterBuild()
{
    Project *project = ProjectManager::startupProject();
    if (!project)
        return RunAfterBuildMode::None;

    if (!project->namedSettings(Constants::SK_USE_GLOBAL).isValid())
        return testSettings().runAfterBuildMode();

    TestProjectSettings *projectSettings = AutotestPlugin::projectSettings(project);
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
    if (!testTreeModel->hasTests())
        return;

    const QList<ITestConfiguration *> tests = mode == RunAfterBuildMode::All
            ? testTreeModel->getAllTestCases() : testTreeModel->getSelectedTests();
    runTests(TestRunMode::RunAfterBuild, tests);
}

void TestRunner::onFinished()
{
    if (m_taskTree)
        m_taskTree.release()->deleteLater();
    disconnect(m_stopDebugConnect);
    disconnect(m_targetConnect);
    qDeleteAll(m_selectedTests);
    m_selectedTests.clear();
    m_cancelTimer.stop();
    m_runMode = TestRunMode::None;
    emit testRunFinished();
}

void TestRunner::reportResult(ResultType type, const QString &description)
{
    TestResult result({}, {});
    result.setResult(type);
    result.setDescription(description);
    emit testResultReady(result);
}

/*************************************************************************************************/

RunConfigurationSelectionDialog::RunConfigurationSelectionDialog(const QString &buildTargetKey,
                                                                 QWidget *parent)
    : QDialog(parent)
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

    if (auto project = ProjectManager::startupProject()) {
        if (auto target = project->activeTarget()) {
            for (RunConfiguration *rc : target->runConfigurations()) {
                auto runnable = rc->runnable();
                const QStringList rcDetails = { runnable.command.executable().toString(),
                                                runnable.command.arguments(),
                                                runnable.workingDirectory.toString() };
                m_rcCombo->addItem(rc->displayName(), rcDetails);
            }
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

} // namespace Internal
} // namespace Autotest
