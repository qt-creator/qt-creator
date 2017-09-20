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
#include "testresultspane.h"
#include "testrunconfiguration.h"
#include "testsettings.h"
#include "testoutputreader.h"
#include "testtreeitem.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/outputformat.h>
#include <utils/runextensions.h>
#include <utils/hostosinfo.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFuture>
#include <QFutureInterface>
#include <QLabel>
#include <QPushButton>
#include <QTime>

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerruncontrol.h>

#include <utils/algorithm.h>

namespace Autotest {
namespace Internal {

static TestRunner *s_instance = nullptr;

TestRunner *TestRunner::instance()
{
    if (!s_instance)
        s_instance = new TestRunner;
    return s_instance;
}

TestRunner::TestRunner(QObject *parent) :
    QObject(parent),
    m_executingTests(false)
{
    connect(&m_futureWatcher, &QFutureWatcher<TestResultPtr>::resultReadyAt,
            this, [this](int index) { emit testResultReady(m_futureWatcher.resultAt(index)); });
    connect(&m_futureWatcher, &QFutureWatcher<TestResultPtr>::finished,
            this, &TestRunner::onFinished);
    connect(this, &TestRunner::requestStopTestRun,
            &m_futureWatcher, &QFutureWatcher<TestResultPtr>::cancel);
    connect(&m_futureWatcher, &QFutureWatcher<TestResultPtr>::canceled,
            this, [this]() {
        emit testResultReady(TestResultPtr(new FaultyTestResult(
                Result::MessageFatal, tr("Test run canceled by user."))));
        m_executingTests = false; // avoid being stuck if finished() signal won't get emitted
    });
}

TestRunner::~TestRunner()
{
    qDeleteAll(m_selectedTests);
    m_selectedTests.clear();
    s_instance = nullptr;
}

void TestRunner::setSelectedTests(const QList<TestConfiguration *> &selected)
{
     qDeleteAll(m_selectedTests);
     m_selectedTests.clear();
     m_selectedTests = selected;
}

void TestRunner::runTest(TestRunMode mode, const TestTreeItem *item)
{
    TestConfiguration *configuration;
    switch (mode) {
    case TestRunMode::Run:
    case TestRunMode::RunWithoutDeploy:
        configuration = item->testConfiguration();
        break;
    case TestRunMode::Debug:
    case TestRunMode::DebugWithoutDeploy:
        configuration = item->debugConfiguration();
        break;
    default:
        configuration = nullptr;
    }

    if (configuration) {
        setSelectedTests({configuration});
        prepareToRunTests(mode);
    }
}

static QString processInformation(const QProcess &proc)
{
    QString information("\nCommand line: " + proc.program() + ' ' + proc.arguments().join(' '));
    QStringList important = { "PATH" };
    if (Utils::HostOsInfo::isLinuxHost())
        important.append("LD_LIBRARY_PATH");
    else if (Utils::HostOsInfo::isMacHost())
        important.append({ "DYLD_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH" });
    const QProcessEnvironment &environment = proc.processEnvironment();
    for (const QString &var : important)
        information.append('\n' + var + ": " + environment.value(var));
    return information;
}

static QString rcInfo(const TestConfiguration * const config)
{
    QString info = '\n' + TestRunner::tr("Run configuration:") + ' ';
    if (config->isGuessed())
        info += TestRunner::tr("guessed from");
    return info + " \"" + config->runConfigDisplayName() + '"';
}

static QString constructOmittedDetailsString(const QStringList &omitted)
{
    QString details = TestRunner::tr("Omitted the following arguments specified on the run "
                                     "configuration page for \"%1\":");
    for (const QString &arg : omitted)
        details += "\n" + arg;
    return details;
}

static void performTestRun(QFutureInterface<TestResultPtr> &futureInterface,
                           const QList<TestConfiguration *> selectedTests,
                           const TestSettings &settings)
{
    const int timeout = settings.timeout;
    const bool omitRunConfigWarnings = settings.omitRunConfigWarn;
    QEventLoop eventLoop;
    int testCaseCount = 0;
    for (TestConfiguration *config : selectedTests) {
        config->completeTestInformation(TestRunMode::Run);
        if (config->project()) {
            testCaseCount += config->testCaseCount();
            if (!omitRunConfigWarnings && config->isGuessed()) {
                QString message = TestRunner::tr(
                            "Project's run configuration was guessed for \"%1\".\n"
                            "This might cause trouble during execution.\n"
                            "(guessed from \"%2\")");
                message = message.arg(config->displayName()).arg(config->runConfigDisplayName());
                futureInterface.reportResult(
                            TestResultPtr(new FaultyTestResult(Result::MessageWarn, message)));
            }
        } else {
            futureInterface.reportResult(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
                TestRunner::tr("Project is null for \"%1\". Removing from test run.\n"
                            "Check the test environment.").arg(config->displayName()))));
        }
    }

    QProcess testProcess;
    testProcess.setReadChannel(QProcess::StandardOutput);

    futureInterface.setProgressRange(0, testCaseCount);
    futureInterface.setProgressValue(0);

    for (const TestConfiguration *testConfiguration : selectedTests) {
        QScopedPointer<TestOutputReader> outputReader;
        outputReader.reset(testConfiguration->outputReader(futureInterface, &testProcess));
        QTC_ASSERT(outputReader, continue);
        TestRunner::connect(outputReader.data(), &TestOutputReader::newOutputAvailable,
                            TestResultsPane::instance(), &TestResultsPane::addOutput);
        if (futureInterface.isCanceled())
            break;

        if (!testConfiguration->project())
            continue;

        QProcessEnvironment environment = testConfiguration->environment().toProcessEnvironment();
        QString commandFilePath = testConfiguration->executableFilePath();
        if (commandFilePath.isEmpty()) {
            futureInterface.reportResult(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                TestRunner::tr("Executable path is empty. (%1)")
                                                   .arg(testConfiguration->displayName()))));
            continue;
        }

        QStringList omitted;
        testProcess.setArguments(testConfiguration->argumentsForTestRunner(&omitted));
        if (!omitted.isEmpty()) {
            const QString &details = constructOmittedDetailsString(omitted);
            futureInterface.reportResult(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
                details.arg(testConfiguration->displayName()))));
        }
        testProcess.setWorkingDirectory(testConfiguration->workingDirectory());
        if (Utils::HostOsInfo::isWindowsHost())
            environment.insert("QT_LOGGING_TO_CONSOLE", "1");
        testProcess.setProcessEnvironment(environment);
        testProcess.setProgram(commandFilePath);
        testProcess.start();

        bool ok = testProcess.waitForStarted();
        QTime executionTimer;
        executionTimer.start();
        bool canceledByTimeout = false;
        if (ok) {
            while (testProcess.state() == QProcess::Running) {
                if (executionTimer.elapsed() >= timeout) {
                    canceledByTimeout = true;
                    break;
                }
                if (futureInterface.isCanceled()) {
                    testProcess.kill();
                    testProcess.waitForFinished();
                    return;
                }
                eventLoop.processEvents();
            }
        } else {
            futureInterface.reportResult(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                TestRunner::tr("Failed to start test for project \"%1\".")
                    .arg(testConfiguration->displayName()) + processInformation(testProcess)
                                                           + rcInfo(testConfiguration))));
        }
        if (testProcess.exitStatus() == QProcess::CrashExit) {
            futureInterface.reportResult(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                TestRunner::tr("Test for project \"%1\" crashed.")
                    .arg(testConfiguration->displayName()) + processInformation(testProcess)
                                                           + rcInfo(testConfiguration))));
        } else if (!outputReader->hadValidOutput()) {
            futureInterface.reportResult(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                TestRunner::tr("Test for project \"%1\" did not produce any expected output.")
                    .arg(testConfiguration->displayName()) + processInformation(testProcess)
                                                           + rcInfo(testConfiguration))));
        }

        if (canceledByTimeout) {
            if (testProcess.state() != QProcess::NotRunning) {
                testProcess.kill();
                testProcess.waitForFinished();
            }
            futureInterface.reportResult(TestResultPtr(
                    new FaultyTestResult(Result::MessageFatal, TestRunner::tr(
                    "Test case canceled due to timeout.\nMaybe raise the timeout?"))));
        }
    }
    futureInterface.setProgressValue(testCaseCount);
}

void TestRunner::prepareToRunTests(TestRunMode mode)
{
    m_runMode = mode;
    ProjectExplorer::Internal::ProjectExplorerSettings projectExplorerSettings =
        ProjectExplorer::ProjectExplorerPlugin::projectExplorerSettings();
    if (projectExplorerSettings.buildBeforeDeploy && !projectExplorerSettings.saveBeforeBuild) {
        if (!ProjectExplorer::ProjectExplorerPlugin::saveModifiedFiles())
            return;
    }

    m_executingTests = true;
    emit testRunStarted();

    // clear old log and output pane
    TestResultsPane::instance()->clearContents();

    if (m_selectedTests.empty()) {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
            tr("No tests selected. Canceling test run."))));
        onFinished();
        return;
    }

    ProjectExplorer::Project *project = m_selectedTests.at(0)->project();
    if (!project) {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
            tr("Project is null. Canceling test run.\n"
            "Only desktop kits are supported. Make sure the "
            "currently active kit is a desktop kit."))));
        onFinished();
        return;
    }

    if (!projectExplorerSettings.buildBeforeDeploy || mode == TestRunMode::DebugWithoutDeploy
            || mode == TestRunMode::RunWithoutDeploy) {
        runOrDebugTests();
    } else  if (project->hasActiveBuildSettings()) {
        buildProject(project);
    } else {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                                           tr("Project is not configured. Canceling test run."))));
        onFinished();
    }
}

static QString firstTestCaseTarget(const TestConfiguration *config)
{
    const QSet<QString> &internalTargets = config->internalTargets();
    int size = internalTargets.size();
    if (size)
        return (*internalTargets.begin()).split('|').first();
    return TestRunner::tr("<unknown>");
}

static bool askUserForRunConfiguration(TestConfiguration *config)
{
    using namespace ProjectExplorer;
    RunConfigurationSelectionDialog dialog(firstTestCaseTarget(config),
                                           Core::ICore::dialogParent());
    if (dialog.exec() == QDialog::Accepted) {
        const QString dName = dialog.displayName();
        if (dName.isEmpty())
            return false;
        // run configuration has been selected - fill config based on this one..
        const QString exe = dialog.executable();
        // paranoia... can the current startup project have changed meanwhile?
        if (auto project = SessionManager::startupProject()) {
            if (auto target = project->activeTarget()) {
                RunConfiguration *runConfig
                        = Utils::findOr(target->runConfigurations(), nullptr,
                                        [&dName, &exe] (const RunConfiguration *rc) {
                    if (rc->displayName() != dName)
                        return false;
                    if (!rc->runnable().is<StandardRunnable>())
                        return false;
                    StandardRunnable runnable = rc->runnable().as<StandardRunnable>();
                    return runnable.executable == exe;
                });
                if (runConfig) {
                    config->setOriginalRunConfiguration(runConfig);
                    return true;
                }
            }
        }
    }
    return false;
}

void TestRunner::runTests()
{
    QList<TestConfiguration *> toBeRemoved;
    for (TestConfiguration *config : m_selectedTests) {
        config->completeTestInformation(TestRunMode::Run);
        if (!config->hasExecutable())
            if (!askUserForRunConfiguration(config))
                toBeRemoved.append(config);
    }
    for (TestConfiguration *config : toBeRemoved)
        m_selectedTests.removeOne(config);
    qDeleteAll(toBeRemoved);
    toBeRemoved.clear();
    if (m_selectedTests.isEmpty()) {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
                tr("No test cases left for execution. Canceling test run."))));
        onFinished();
        return;
    }

    QFuture<TestResultPtr> future = Utils::runAsync(&performTestRun, m_selectedTests,
                                                    *AutotestPlugin::instance()->settings());
    m_futureWatcher.setFuture(future);
    Core::ProgressManager::addTask(future, tr("Running Tests"), Autotest::Constants::TASK_INDEX);
}

static void processOutput(TestOutputReader *outputreader, const QString &msg,
                          Utils::OutputFormat format)
{
    switch (format) {
    case Utils::OutputFormat::StdOutFormatSameLine:
    case Utils::OutputFormat::DebugFormat: {
        static const QString gdbSpecialOut = "Qt: gdb: -nograb added to command-line options.\n"
                                             "\t Use the -dograb option to enforce grabbing.";
        int start = msg.startsWith(gdbSpecialOut) ? gdbSpecialOut.length() + 1 : 0;
        if (start) {
            int maxIndex = msg.length() - 1;
            while (start < maxIndex && msg.at(start + 1) == '\n')
                ++start;
            if (start >= msg.length()) // we cut out the whole message
                break;
        }
        for (const QString &line : msg.mid(start).split('\n'))
            outputreader->processOutput(line.toUtf8());
        break;
    }
    case Utils::OutputFormat::StdErrFormatSameLine:
        outputreader->processStdError(msg.toUtf8());
        break;
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
    if (!config->hasExecutable()) {
        if (askUserForRunConfiguration(config))
            config->completeTestInformation(config->originalRunConfiguration(), TestRunMode::Debug);
    }

    if (!config->runConfiguration()) {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
            TestRunner::tr("Failed to get run configuration."))));
        onFinished();
        return;
    }

    const QString &commandFilePath = config->executableFilePath();
    if (commandFilePath.isEmpty()) {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
            TestRunner::tr("Could not find command \"%1\". (%2)")
                                               .arg(config->executableFilePath())
                                               .arg(config->displayName()))));
        onFinished();
        return;
    }

    QString errorMessage;
    auto runControl = new ProjectExplorer::RunControl(config->runConfiguration(),
                                                      ProjectExplorer::Constants::DEBUG_RUN_MODE);
    if (!runControl) {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
            TestRunner::tr("Failed to create run configuration.\n%1").arg(errorMessage))));
        onFinished();
        return;
    }

    QStringList omitted;
    ProjectExplorer::StandardRunnable inferior = config->runnable();
    inferior.executable = commandFilePath;
    inferior.commandLineArguments = config->argumentsForTestRunner(&omitted).join(' ');
    if (!omitted.isEmpty()) {
        const QString &details = constructOmittedDetailsString(omitted);
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
            details.arg(config->displayName()))));
    }
    auto debugger = new Debugger::DebuggerRunTool(runControl);
    debugger->setInferior(inferior);
    debugger->setRunControlName(config->displayName());

    bool useOutputProcessor = true;
    if (ProjectExplorer::Target *targ = config->project()->activeTarget()) {
        if (Debugger::DebuggerKitInformation::engineType(targ->kit()) == Debugger::CdbEngineType) {
            emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
                TestRunner::tr("Unable to display test results when using CDB."))));
            useOutputProcessor = false;
        }
    }

    // We need a fake QFuture for the results. TODO: replace with QtConcurrent::run
    QFutureInterface<TestResultPtr> *futureInterface
            = new QFutureInterface<TestResultPtr>(QFutureInterfaceBase::Running);
    QFuture<TestResultPtr> future(futureInterface);
    m_futureWatcher.setFuture(future);

    if (useOutputProcessor) {
        TestOutputReader *outputreader = config->outputReader(*futureInterface, 0);
        connect(outputreader, &TestOutputReader::newOutputAvailable,
                TestResultsPane::instance(), &TestResultsPane::addOutput);
        connect(runControl, &ProjectExplorer::RunControl::appendMessageRequested,
                this, [outputreader]
                (ProjectExplorer::RunControl *, const QString &msg, Utils::OutputFormat format) {
            processOutput(outputreader, msg, format);
        });

        connect(runControl, &ProjectExplorer::RunControl::stopped,
                outputreader, &QObject::deleteLater);
    }

    connect(this, &TestRunner::requestStopTestRun, runControl,
            &ProjectExplorer::RunControl::initiateStop);
    connect(runControl, &ProjectExplorer::RunControl::stopped, this, &TestRunner::onFinished);
    ProjectExplorer::ProjectExplorerPlugin::startRunControl(runControl);
}

void TestRunner::runOrDebugTests()
{
    switch (m_runMode) {
    case TestRunMode::Run:
    case TestRunMode::RunWithoutDeploy:
        runTests();
        break;
    case TestRunMode::Debug:
    case TestRunMode::DebugWithoutDeploy:
        debugTests();
        break;
    default:
        onFinished();
        QTC_ASSERT(false, return);  // unexpected run mode
    }
}

void TestRunner::buildProject(ProjectExplorer::Project *project)
{
    ProjectExplorer::BuildManager *buildManager = ProjectExplorer::BuildManager::instance();
    m_buildConnect = connect(this, &TestRunner::requestStopTestRun,
                             buildManager, &ProjectExplorer::BuildManager::cancel);
    connect(buildManager, &ProjectExplorer::BuildManager::buildQueueFinished,
            this, &TestRunner::buildFinished);
    ProjectExplorer::ProjectExplorerPlugin::buildProject(project);
    if (!buildManager->isBuilding())
        buildFinished(false);
}

void TestRunner::buildFinished(bool success)
{
    disconnect(m_buildConnect);
    ProjectExplorer::BuildManager *buildManager = ProjectExplorer::BuildManager::instance();
    disconnect(buildManager, &ProjectExplorer::BuildManager::buildQueueFinished,
               this, &TestRunner::buildFinished);

    if (success) {
        runOrDebugTests();
    } else {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                                                  tr("Build failed. Canceling test run."))));
        onFinished();
    }
}

void TestRunner::onFinished()
{
    m_executingTests = false;
    emit testRunFinished();
}

/*************************************************************************************************/

RunConfigurationSelectionDialog::RunConfigurationSelectionDialog(const QString &testsInfo,
                                                                 QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Select Run Configuration"));

    m_details = new QLabel(tr("Could not determine which run configuration to choose for running"
                              " tests (%1)").arg(testsInfo), this);
    m_rcCombo = new QComboBox(this);
    m_executable = new QLabel(this);
    m_arguments = new QLabel(this);
    m_workingDir = new QLabel(this);
    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    m_buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    auto line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    auto formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(m_details);
    formLayout->addRow(tr("Run Configuration:"), m_rcCombo);
    formLayout->addRow(line);
    formLayout->addRow(tr("Executable:"), m_executable);
    formLayout->addRow(tr("Arguments:"), m_arguments);
    formLayout->addRow(tr("Working Directory:"), m_workingDir);
    // TODO Device support
    auto vboxLayout = new QVBoxLayout(this);
    vboxLayout->addLayout(formLayout);
    vboxLayout->addStretch();
    vboxLayout->addWidget(line);
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

void RunConfigurationSelectionDialog::populate()
{
    m_rcCombo->addItem(QString(), QStringList({QString(), QString(), QString()})); // empty default

    if (auto project = ProjectExplorer::SessionManager::startupProject()) {
        if (auto target = project->activeTarget()) {
            for (ProjectExplorer::RunConfiguration *rc : target->runConfigurations()) {
                if (rc->runnable().is<ProjectExplorer::StandardRunnable>()) {
                    auto runnable = rc->runnable().as<ProjectExplorer::StandardRunnable>();
                    const QStringList rcDetails = { runnable.executable,
                                                    runnable.commandLineArguments,
                                                    runnable.workingDirectory };
                    m_rcCombo->addItem(rc->displayName(), rcDetails);
                }
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
