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

#include "testrunner.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "testprojectsettings.h"
#include "testresultspane.h"
#include "testrunconfiguration.h"
#include "testsettings.h"
#include "testoutputreader.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/outputformat.h>
#include <utils/qtcprocess.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFuture>
#include <QFutureInterface>
#include <QLabel>
#include <QLoggingCategory>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QTimer>

using namespace ProjectExplorer;
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

    connect(&m_futureWatcher, &QFutureWatcher<TestResultPtr>::resultReadyAt,
            this, [this](int index) { emit testResultReady(m_futureWatcher.resultAt(index)); });
    connect(&m_futureWatcher, &QFutureWatcher<TestResultPtr>::finished,
            this, &TestRunner::onFinished);
    connect(this, &TestRunner::requestStopTestRun,
            &m_futureWatcher, &QFutureWatcher<TestResultPtr>::cancel);
    connect(&m_futureWatcher, &QFutureWatcher<TestResultPtr>::canceled,
            this, [this]() {
        cancelCurrent(UserCanceled);
        reportResult(ResultType::MessageFatal, tr("Test run canceled by user."));
    });
    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            this, &TestRunner::onBuildQueueFinished);
}

TestRunner::~TestRunner()
{
    qDeleteAll(m_selectedTests);
    m_selectedTests.clear();
    s_instance = nullptr;
}

void TestRunner::setSelectedTests(const QList<TestConfiguration *> &selected)
{
    QTC_ASSERT(!m_executingTests, return);
    qDeleteAll(m_selectedTests);
    m_selectedTests.clear();
    m_selectedTests.append(selected);
}

void TestRunner::runTest(TestRunMode mode, const TestTreeItem *item)
{
    QTC_ASSERT(!m_executingTests, return);
    TestConfiguration *configuration = item->asConfiguration(mode);

    if (configuration) {
        setSelectedTests({configuration});
        prepareToRunTests(mode);
    }
}

static QString processInformation(const QProcess *proc)
{
    QTC_ASSERT(proc, return QString());
    QString information("\nCommand line: " + proc->program() + ' ' + proc->arguments().join(' '));
    QStringList important = { "PATH" };
    if (Utils::HostOsInfo::isLinuxHost())
        important.append("LD_LIBRARY_PATH");
    else if (Utils::HostOsInfo::isMacHost())
        important.append({ "DYLD_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH" });
    const QProcessEnvironment &environment = proc->processEnvironment();
    for (const QString &var : important)
        information.append('\n' + var + ": " + environment.value(var));
    return information;
}

static QString rcInfo(const TestConfiguration * const config)
{
    QString info;
    if (config->isDeduced())
        info = TestRunner::tr("\nRun configuration: deduced from \"%1\"");
    else
        info = TestRunner::tr("\nRun configuration: \"%1\"");
    return info.arg(config->runConfigDisplayName());
}

static QString constructOmittedDetailsString(const QStringList &omitted)
{
    return TestRunner::tr("Omitted the following arguments specified on the run "
                          "configuration page for \"%1\":") + '\n' + omitted.join('\n');
}

static QString constructOmittedVariablesDetailsString(const Utils::EnvironmentItems &diff)
{
    auto removedVars = Utils::transform<QStringList>(diff, [](const Utils::EnvironmentItem &it) {
        return it.name;
    });
    return TestRunner::tr("Omitted the following environment variables for \"%1\":")
            + '\n' + removedVars.join('\n');
}

void TestRunner::scheduleNext()
{
    QTC_ASSERT(!m_selectedTests.isEmpty(), onFinished(); return);
    QTC_ASSERT(!m_currentConfig && !m_currentProcess, resetInternalPointers());
    QTC_ASSERT(m_fakeFutureInterface, onFinished(); return);
    QTC_ASSERT(!m_canceled, onFinished(); return);

    m_currentConfig = m_selectedTests.dequeue();

    QString commandFilePath = m_currentConfig->executableFilePath();
    if (commandFilePath.isEmpty()) {
        reportResult(ResultType::MessageFatal,
            tr("Executable path is empty. (%1)").arg(m_currentConfig->displayName()));
        delete m_currentConfig;
        m_currentConfig = nullptr;
        if (m_selectedTests.isEmpty()) {
            if (m_fakeFutureInterface)
                m_fakeFutureInterface->reportFinished();
            onFinished();
        } else {
            onProcessFinished();
        }
        return;
    }
    if (!m_currentConfig->project())
        onProcessFinished();

    m_currentProcess = new QProcess;
    m_currentProcess->setReadChannel(QProcess::StandardOutput);
    m_currentProcess->setProgram(commandFilePath);

    QTC_ASSERT(!m_currentOutputReader, delete m_currentOutputReader);
    m_currentOutputReader = m_currentConfig->outputReader(*m_fakeFutureInterface, m_currentProcess);
    QTC_ASSERT(m_currentOutputReader, onProcessFinished();return);

    connect(m_currentOutputReader, &TestOutputReader::newOutputLineAvailable,
            TestResultsPane::instance(), &TestResultsPane::addOutputLine);


    QStringList omitted;
    m_currentProcess->setArguments(m_currentConfig->argumentsForTestRunner(&omitted));
    if (!omitted.isEmpty()) {
        const QString &details = constructOmittedDetailsString(omitted);
        reportResult(ResultType::MessageWarn, details.arg(m_currentConfig->displayName()));
    }
    m_currentProcess->setWorkingDirectory(m_currentConfig->workingDirectory());
    const Utils::Environment &original = m_currentConfig->environment();
    Utils::Environment environment =  m_currentConfig->filteredEnvironment(original);
    const Utils::EnvironmentItems removedVariables = Utils::filtered(
        original.diff(environment), [](const Utils::EnvironmentItem &it) {
            return it.operation == Utils::EnvironmentItem::Unset;
        });
    if (!removedVariables.isEmpty()) {
        const QString &details = constructOmittedVariablesDetailsString(removedVariables)
                .arg(m_currentConfig->displayName());
        reportResult(ResultType::MessageWarn, details);
    }
    m_currentProcess->setProcessEnvironment(environment.toProcessEnvironment());

    connect(m_currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TestRunner::onProcessFinished);
    const int timeout = AutotestPlugin::settings()->timeout;
    QTimer::singleShot(timeout, m_currentProcess, [this]() { cancelCurrent(Timeout); });

    qCInfo(runnerLog) << "Command:" << m_currentProcess->program();
    qCInfo(runnerLog) << "Arguments:" << m_currentProcess->arguments();
    qCInfo(runnerLog) << "Working directory:" << m_currentProcess->workingDirectory();
    qCDebug(runnerLog) << "Environment:" << m_currentProcess->environment();

    m_currentProcess->start();
    if (!m_currentProcess->waitForStarted()) {
        reportResult(ResultType::MessageFatal,
            tr("Failed to start test for project \"%1\".").arg(m_currentConfig->displayName())
                + processInformation(m_currentProcess) + rcInfo(m_currentConfig));
    }
}

void TestRunner::cancelCurrent(TestRunner::CancelReason reason)
{
    m_canceled = true;

    if (m_fakeFutureInterface)
        m_fakeFutureInterface->reportCanceled();

    if (reason == KitChanged)
        reportResult(ResultType::MessageWarn, tr("Current kit has changed. Canceling test run."));
    else if (reason == Timeout)
        reportResult(ResultType::MessageFatal, tr("Test case canceled due to timeout.\nMaybe raise the timeout?"));

    // if user or timeout cancels the current run ensure to kill the running process
    if (m_currentProcess && m_currentProcess->state() != QProcess::NotRunning) {
        m_currentProcess->kill();
        m_currentProcess->waitForFinished();
    }
}

void TestRunner::onProcessFinished()
{
    if (m_executingTests && QTC_GUARD(m_currentConfig)) {
        QTC_CHECK(m_fakeFutureInterface);
        m_fakeFutureInterface->setProgressValue(m_fakeFutureInterface->progressValue()
                                                + m_currentConfig->testCaseCount());
        if (!m_fakeFutureInterface->isCanceled()) {
            if (m_currentProcess->exitStatus() == QProcess::CrashExit) {
                m_currentOutputReader->reportCrash();
                reportResult(ResultType::MessageFatal,
                        tr("Test for project \"%1\" crashed.").arg(m_currentConfig->displayName())
                        + processInformation(m_currentProcess) + rcInfo(m_currentConfig));
            } else if (!m_currentOutputReader->hadValidOutput()) {
                reportResult(ResultType::MessageFatal,
                    tr("Test for project \"%1\" did not produce any expected output.")
                    .arg(m_currentConfig->displayName()) + processInformation(m_currentProcess)
                    + rcInfo(m_currentConfig));
            }
        }
    }
    const int disabled = m_currentOutputReader->disabledTests();
    if (disabled > 0)
        emit hadDisabledTests(disabled);
    if (m_currentOutputReader->hasSummary())
        emit reportSummary(m_currentOutputReader->id(), m_currentOutputReader->summary());

    m_currentOutputReader->resetCommandlineColor();
    resetInternalPointers();

    if (!m_fakeFutureInterface) {
        QTC_ASSERT(!m_executingTests, m_executingTests = false);
        return;
    }
    if (!m_selectedTests.isEmpty() && !m_fakeFutureInterface->isCanceled())
        scheduleNext();
    else
        m_fakeFutureInterface->reportFinished();
}

void TestRunner::resetInternalPointers()
{
    delete m_currentOutputReader;
    delete m_currentProcess;
    delete m_currentConfig;
    m_currentOutputReader = nullptr;
    m_currentProcess = nullptr;
    m_currentConfig = nullptr;
}

void TestRunner::prepareToRunTests(TestRunMode mode)
{
    QTC_ASSERT(!m_executingTests, return);
    m_skipTargetsCheck = false;
    m_runMode = mode;
    ProjectExplorer::Internal::ProjectExplorerSettings projectExplorerSettings =
        ProjectExplorerPlugin::projectExplorerSettings();
    if (mode != TestRunMode::RunAfterBuild
            && projectExplorerSettings.buildBeforeDeploy != ProjectExplorer::Internal::BuildBeforeRunMode::Off
            && !projectExplorerSettings.saveBeforeBuild) {
        if (!ProjectExplorerPlugin::saveModifiedFiles())
            return;
    }

    m_executingTests = true;
    m_canceled = false;
    emit testRunStarted();

    // clear old log and output pane
    TestResultsPane::instance()->clearContents();

    if (m_selectedTests.empty()) {
        reportResult(ResultType::MessageWarn, tr("No tests selected. Canceling test run."));
        onFinished();
        return;
    }

    Project *project = m_selectedTests.at(0)->project();
    if (!project) {
        reportResult(ResultType::MessageWarn,
            tr("Project is null. Canceling test run.\n"
               "Only desktop kits are supported. Make sure the "
               "currently active kit is a desktop kit."));
        onFinished();
        return;
    }

    m_targetConnect = connect(project, &Project::activeTargetChanged,
                              [this]() { cancelCurrent(KitChanged); });

    if (projectExplorerSettings.buildBeforeDeploy == ProjectExplorer::Internal::BuildBeforeRunMode::Off
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
                     tr("Project is not configured. Canceling test run."));
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
    const Project *project = SessionManager::startupProject();
    if (!project)
        return nullptr;
    const Target *target = project->activeTarget();
    if (!target)
        return nullptr;

    RunConfiguration *runConfig = nullptr;
    const QList<RunConfiguration *> runConfigurations
            = Utils::filtered(target->runConfigurations(), [] (const RunConfiguration *rc) {
        return !rc->runnable().executable.isEmpty();
    });

    const ChoicePair oldChoice = AutotestPlugin::cachedChoiceFor(buildTargetKey);
    if (!oldChoice.executable.isEmpty()) {
        runConfig = Utils::findOrDefault(runConfigurations,
                                  [&oldChoice] (const RunConfiguration *rc) {
            return oldChoice.matches(rc);
        });
        if (runConfig)
            return runConfig;
    }

    if (runConfigurations.size() == 1)
        return runConfigurations.first();

    RunConfigurationSelectionDialog dialog(buildTargetKey, Core::ICore::dialogParent());
    if (dialog.exec() == QDialog::Accepted) {
        const QString dName = dialog.displayName();
        if (dName.isEmpty())
            return nullptr;
        // run configuration has been selected - fill config based on this one..
        const QString exe = dialog.executable();
        runConfig = Utils::findOr(runConfigurations, nullptr, [&dName, &exe] (const RunConfiguration *rc) {
            if (rc->displayName() != dName)
                return false;
            return rc->runnable().executable.toString() == exe;
        });
        if (runConfig && dialog.rememberChoice())
            AutotestPlugin::cacheRunConfigChoice(buildTargetKey, ChoicePair(dName, exe));
    }
    return runConfig;
}

int TestRunner::precheckTestConfigurations()
{
    const bool omitWarnings = AutotestPlugin::settings()->omitRunConfigWarn;
    int testCaseCount = 0;
    for (TestConfiguration *config : m_selectedTests) {
        config->completeTestInformation(TestRunMode::Run);
        if (config->project()) {
            testCaseCount += config->testCaseCount();
            if (!omitWarnings && config->isDeduced()) {
                QString message = tr(
                            "Project's run configuration was deduced for \"%1\".\n"
                            "This might cause trouble during execution.\n"
                            "(deduced from \"%2\")");
                message = message.arg(config->displayName()).arg(config->runConfigDisplayName());
                reportResult(ResultType::MessageWarn, message);
            }
        } else {
            reportResult(ResultType::MessageWarn,
                         tr("Project is null for \"%1\". Removing from test run.\n"
                            "Check the test environment.").arg(config->displayName()));
        }
    }
    return testCaseCount;
}

void TestRunner::onBuildSystemUpdated()
{
    Target *target = SessionManager::startupTarget();
    if (QTC_GUARD(target))
        disconnect(target, &Target::buildSystemUpdated, this, &TestRunner::onBuildSystemUpdated);
    if (!m_skipTargetsCheck) {
        m_skipTargetsCheck = true;
        runOrDebugTests();
    }
}

void TestRunner::runTests()
{
    QList<TestConfiguration *> toBeRemoved;
    bool projectChanged = false;
    for (TestConfiguration *config : m_selectedTests) {
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
    for (TestConfiguration *config : toBeRemoved)
        m_selectedTests.removeOne(config);
    qDeleteAll(toBeRemoved);
    toBeRemoved.clear();
    if (m_selectedTests.isEmpty()) {
        QString mssg = projectChanged ? tr("Startup project has changed. Canceling test run.")
                                      : tr("No test cases left for execution. Canceling test run.");

        reportResult(ResultType::MessageWarn, mssg);
        onFinished();
        return;
    }

    int testCaseCount = precheckTestConfigurations();

    // Fake future interface - destruction will be handled by QFuture/QFutureWatcher
    m_fakeFutureInterface = new QFutureInterface<TestResultPtr>(QFutureInterfaceBase::Running);
    QFuture<TestResultPtr> future = m_fakeFutureInterface->future();
    m_fakeFutureInterface->setProgressRange(0, testCaseCount);
    m_fakeFutureInterface->setProgressValue(0);
    m_futureWatcher.setFuture(future);

    Core::ProgressManager::addTask(future, tr("Running Tests"), Autotest::Constants::TASK_INDEX);
    if (AutotestPlugin::settings()->popupOnStart)
        AutotestPlugin::popupResultsPane();
    scheduleNext();
}

static void processOutput(TestOutputReader *outputreader, const QString &msg,
                          Utils::OutputFormat format)
{
    QByteArray message = msg.toUtf8();
    switch (format) {
    case Utils::OutputFormat::StdErrFormat:
    case Utils::OutputFormat::StdOutFormat:
    case Utils::OutputFormat::DebugFormat: {
        static const QByteArray gdbSpecialOut = "Qt: gdb: -nograb added to command-line options.\n"
                                                "\t Use the -dograb option to enforce grabbing.";
        if (message.startsWith(gdbSpecialOut))
            message = message.mid(gdbSpecialOut.length() + 1);
        message.chop(1); // all messages have an additional \n at the end

        for (const auto &line : message.split('\n')) {
            if (format == Utils::OutputFormat::StdOutFormat)
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
    QTC_ASSERT(m_selectedTests.size() == 1, onFinished();return);

    TestConfiguration *config = m_selectedTests.first();
    config->completeTestInformation(TestRunMode::Debug);
    if (!config->project()) {
        reportResult(ResultType::MessageWarn,
                     tr("Startup project has changed. Canceling test run."));
        onFinished();
        return;
    }
    if (!config->hasExecutable()) {
        if (auto *rc = getRunConfiguration(firstNonEmptyTestCaseTarget(config)))
            config->completeTestInformation(rc, TestRunMode::Debug);
    }

    if (!config->runConfiguration()) {
        reportResult(ResultType::MessageFatal, tr("Failed to get run configuration."));
        onFinished();
        return;
    }

    const QString &commandFilePath = config->executableFilePath();
    if (commandFilePath.isEmpty()) {
        reportResult(ResultType::MessageFatal, tr("Could not find command \"%1\". (%2)")
                     .arg(config->executableFilePath())
                     .arg(config->displayName()));
        onFinished();
        return;
    }

    QString errorMessage;
    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setRunConfiguration(config->runConfiguration());

    QStringList omitted;
    Runnable inferior = config->runnable();
    inferior.executable = FilePath::fromString(commandFilePath);

    const QStringList args = config->argumentsForTestRunner(&omitted);
    inferior.commandLineArguments = Utils::QtcProcess::joinArgs(args);
    if (!omitted.isEmpty()) {
        const QString &details = constructOmittedDetailsString(omitted);
        reportResult(ResultType::MessageWarn, details.arg(config->displayName()));
    }
    Utils::Environment original(inferior.environment);
    inferior.environment = config->filteredEnvironment(original);
    const Utils::EnvironmentItems removedVariables = Utils::filtered(
        original.diff(inferior.environment), [](const Utils::EnvironmentItem &it) {
            return it.operation == Utils::EnvironmentItem::Unset;
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
                         tr("Unable to display test results when using CDB."));
            useOutputProcessor = false;
        }
    }

    // We need a fake QFuture for the results. TODO: replace with QtConcurrent::run
    QFutureInterface<TestResultPtr> *futureInterface
            = new QFutureInterface<TestResultPtr>(QFutureInterfaceBase::Running);
    m_futureWatcher.setFuture(futureInterface->future());

    if (useOutputProcessor) {
        TestOutputReader *outputreader = config->outputReader(*futureInterface, nullptr);
        outputreader->setId(inferior.executable.toString());
        connect(outputreader, &TestOutputReader::newOutputLineAvailable,
                TestResultsPane::instance(), &TestResultsPane::addOutputLine);
        connect(runControl, &RunControl::appendMessage,
                this, [outputreader](const QString &msg, Utils::OutputFormat format) {
            processOutput(outputreader, msg, format);
        });

        connect(runControl, &RunControl::stopped,
                outputreader, &QObject::deleteLater);
    }

    m_stopDebugConnect = connect(this, &TestRunner::requestStopTestRun,
                                 runControl, &RunControl::initiateStop);

    connect(runControl, &RunControl::stopped, this, &TestRunner::onFinished);
    m_finishDebugConnect = connect(runControl, &RunControl::finished, this, &TestRunner::onFinished);
    ProjectExplorerPlugin::startRunControl(runControl);
    if (useOutputProcessor && AutotestPlugin::settings()->popupOnStart)
        AutotestPlugin::popupResultsPane();
}

static bool executablesEmpty()
{
    Target *target = SessionManager::startupTarget();
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
            Target * target = SessionManager::startupTarget();
            QTimer::singleShot(5000, this, [this, target = QPointer<Target>(target)]() {
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
        runTests();
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
    connect(buildManager, &BuildManager::buildQueueFinished,
            this, &TestRunner::buildFinished);
    buildManager->buildProjectWithDependencies(project);
    if (!buildManager->isBuilding())
        buildFinished(false);
}

void TestRunner::buildFinished(bool success)
{
    disconnect(m_buildConnect);
    BuildManager *buildManager = BuildManager::instance();
    disconnect(buildManager, &BuildManager::buildQueueFinished,
               this, &TestRunner::buildFinished);

    if (success) {
        if (!m_canceled)
            runOrDebugTests();
        else if (m_executingTests)
            onFinished();
    } else {
        reportResult(ResultType::MessageFatal, tr("Build failed. Canceling test run."));
        onFinished();
    }
}

static RunAfterBuildMode runAfterBuild()
{
    Project *project = SessionManager::startupProject();
    if (!project)
        return RunAfterBuildMode::None;

    if (!project->namedSettings(Constants::SK_USE_GLOBAL).isValid())
        return AutotestPlugin::settings()->runAfterBuild;

    TestProjectSettings *projectSettings = AutotestPlugin::projectSettings(project);
    return projectSettings->useGlobalSettings() ? AutotestPlugin::settings()->runAfterBuild
                                                : projectSettings->runAfterBuild();
}

void TestRunner::onBuildQueueFinished(bool success)
{
    if (m_executingTests || !m_selectedTests.isEmpty())  // paranoia!
        return;

    if (!success || m_runMode != TestRunMode::None)
        return;

    RunAfterBuildMode mode = runAfterBuild();
    if (mode == RunAfterBuildMode::None)
        return;

    auto testTreeModel = TestTreeModel::instance();
    if (!testTreeModel->hasTests())
        return;

    setSelectedTests(mode == RunAfterBuildMode::All ? testTreeModel->getAllTestCases()
                                                    : testTreeModel->getSelectedTests());
    prepareToRunTests(TestRunMode::RunAfterBuild);
}

void TestRunner::onFinished()
{
    // if we've been canceled and we still have test configurations queued just throw them away
    qDeleteAll(m_selectedTests);
    m_selectedTests.clear();

    disconnect(m_stopDebugConnect);
    disconnect(m_finishDebugConnect);
    disconnect(m_targetConnect);
    m_fakeFutureInterface = nullptr;
    m_runMode = TestRunMode::None;
    m_executingTests = false;
    emit testRunFinished();
}

void TestRunner::reportResult(ResultType type, const QString &description)
{
    TestResultPtr result(new TestResult);
    result->setResult(type);
    result->setDescription(description);
    emit testResultReady(result);
}

/*************************************************************************************************/

static QFrame *createLine(QWidget *parent)
{
    QFrame *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

RunConfigurationSelectionDialog::RunConfigurationSelectionDialog(const QString &buildTargetKey,
                                                                 QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Select Run Configuration"));

    QString details = tr("Could not determine which run configuration to choose for running tests");
    if (!buildTargetKey.isEmpty())
        details.append(QString(" (%1)").arg(buildTargetKey));
    m_details = new QLabel(details, this);
    m_rcCombo = new QComboBox(this);
    m_rememberCB = new QCheckBox(tr("Remember choice. Cached choices can be reset by switching "
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
    formLayout->addRow(tr("Run Configuration:"), m_rcCombo);
    formLayout->addRow(m_rememberCB);
    formLayout->addRow(createLine(this));
    formLayout->addRow(tr("Executable:"), m_executable);
    formLayout->addRow(tr("Arguments:"), m_arguments);
    formLayout->addRow(tr("Working Directory:"), m_workingDir);
    // TODO Device support
    auto vboxLayout = new QVBoxLayout(this);
    vboxLayout->addLayout(formLayout);
    vboxLayout->addStretch();
    vboxLayout->addWidget(createLine(this));
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
    m_rcCombo->addItem(QString(), QStringList({QString(), QString(), QString()})); // empty default

    if (auto project = SessionManager::startupProject()) {
        if (auto target = project->activeTarget()) {
            for (RunConfiguration *rc : target->runConfigurations()) {
                auto runnable = rc->runnable();
                const QStringList rcDetails = { runnable.executable.toString(),
                                                runnable.commandLineArguments,
                                                runnable.workingDirectory };
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
