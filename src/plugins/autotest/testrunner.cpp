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

#include <utils/hostosinfo.h>
#include <utils/outputformat.h>
#include <utils/qtcprocess.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFuture>
#include <QFutureInterface>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QTimer>

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
        cancelCurrent(UserCanceled);
        emit testResultReady(TestResultPtr(new FaultyTestResult(
                Result::MessageFatal, tr("Test run canceled by user."))));
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
    QString info = '\n' + TestRunner::tr("Run configuration:") + ' ';
    if (config->isGuessed())
        info += TestRunner::tr("guessed from");
    return info + " \"" + config->runConfigDisplayName() + '"';
}

static QString constructOmittedDetailsString(const QStringList &omitted)
{
    return TestRunner::tr("Omitted the following arguments specified on the run "
                          "configuration page for \"%1\":") + '\n' + omitted.join('\n');
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
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
            tr("Executable path is empty. (%1)").arg(m_currentConfig->displayName()))));
        delete m_currentConfig;
        m_currentConfig = nullptr;
        if (m_selectedTests.isEmpty())
            onFinished();
        else
            onProcessFinished();
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

    connect(m_currentOutputReader, &TestOutputReader::newOutputAvailable,
            TestResultsPane::instance(), &TestResultsPane::addOutput);


    QStringList omitted;
    m_currentProcess->setArguments(m_currentConfig->argumentsForTestRunner(&omitted));
    if (!omitted.isEmpty()) {
        const QString &details = constructOmittedDetailsString(omitted);
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
            details.arg(m_currentConfig->displayName()))));
    }
    m_currentProcess->setWorkingDirectory(m_currentConfig->workingDirectory());
    QProcessEnvironment environment = m_currentConfig->environment().toProcessEnvironment();
    if (Utils::HostOsInfo::isWindowsHost())
        environment.insert("QT_LOGGING_TO_CONSOLE", "1");
    const int timeout = AutotestPlugin::settings()->timeout;
    if (timeout > 5 * 60 * 1000) // Qt5.5 introduced hard limit, Qt5.6.1 added env var to raise this
        environment.insert("QTEST_FUNCTION_TIMEOUT", QString::number(timeout));
    m_currentProcess->setProcessEnvironment(environment);

    connect(m_currentProcess,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &TestRunner::onProcessFinished);
    QTimer::singleShot(timeout, m_currentProcess, [this]() { cancelCurrent(Timeout); });

    m_currentProcess->start();
    if (!m_currentProcess->waitForStarted()) {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
            tr("Failed to start test for project \"%1\".").arg(m_currentConfig->displayName())
                + processInformation(m_currentProcess) + rcInfo(m_currentConfig))));
    }
}

void TestRunner::cancelCurrent(TestRunner::CancelReason reason)
{
    m_canceled = true;
    if (reason == UserCanceled) {
        // when using the stop button we need to report, for progress bar this happens automatically
        if (m_fakeFutureInterface && !m_fakeFutureInterface->isCanceled())
            m_fakeFutureInterface->reportCanceled();
    } else if (reason == KitChanged) {
        if (m_fakeFutureInterface)
            m_fakeFutureInterface->reportCanceled();
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
                tr("Current kit has changed. Canceling test run."))));
    }
    if (m_currentProcess && m_currentProcess->state() != QProcess::NotRunning) {
        m_currentProcess->kill();
        m_currentProcess->waitForFinished();
    }
    if (reason == Timeout) {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                tr("Test case canceled due to timeout.\nMaybe raise the timeout?"))));
    }
}

void TestRunner::onProcessFinished()
{
    if (m_currentConfig) {
        m_fakeFutureInterface->setProgressValue(m_fakeFutureInterface->progressValue()
                                                + m_currentConfig->testCaseCount());
        if (!m_fakeFutureInterface->isCanceled()) {
            if (m_currentProcess->exitStatus() == QProcess::CrashExit) {
                m_currentOutputReader->reportCrash();
                emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                        tr("Test for project \"%1\" crashed.").arg(m_currentConfig->displayName())
                        + processInformation(m_currentProcess) + rcInfo(m_currentConfig))));
            } else if (!m_currentOutputReader->hadValidOutput()) {
                emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                    tr("Test for project \"%1\" did not produce any expected output.")
                    .arg(m_currentConfig->displayName()) + processInformation(m_currentProcess)
                    + rcInfo(m_currentConfig))));
            }
        }
    }
    resetInternalPointers();

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
    m_runMode = mode;
    ProjectExplorer::Internal::ProjectExplorerSettings projectExplorerSettings =
        ProjectExplorer::ProjectExplorerPlugin::projectExplorerSettings();
    if (projectExplorerSettings.buildBeforeDeploy && !projectExplorerSettings.saveBeforeBuild) {
        if (!ProjectExplorer::ProjectExplorerPlugin::saveModifiedFiles())
            return;
    }

    m_executingTests = true;
    m_canceled = false;
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

    m_targetConnect = connect(project, &ProjectExplorer::Project::activeTargetChanged,
                              [this]() { cancelCurrent(KitChanged); });

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
    for (const QString &internalTarget : config->internalTargets()) {
        const QString buildTarget = internalTarget.split('|').first();
        if (!buildTarget.isEmpty())
            return buildTarget;
    }
    return QString();
}

static ProjectExplorer::RunConfiguration *getRunConfiguration(const QString &dialogDetail)
{
    using namespace ProjectExplorer;
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
    if (runConfigurations.size() == 1)
        return runConfigurations.first();

    RunConfigurationSelectionDialog dialog(dialogDetail, Core::ICore::dialogParent());
    if (dialog.exec() == QDialog::Accepted) {
        const QString dName = dialog.displayName();
        if (dName.isEmpty())
            return nullptr;
        // run configuration has been selected - fill config based on this one..
        const QString exe = dialog.executable();
        runConfig = Utils::findOr(runConfigurations, nullptr, [&dName, &exe] (const RunConfiguration *rc) {
            if (rc->displayName() != dName)
                return false;
            return rc->runnable().executable == exe;
        });
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
            if (!omitWarnings && config->isGuessed()) {
                QString message = tr(
                            "Project's run configuration was guessed for \"%1\".\n"
                            "This might cause trouble during execution.\n"
                            "(guessed from \"%2\")");
                message = message.arg(config->displayName()).arg(config->runConfigDisplayName());
                emit testResultReady(
                            TestResultPtr(new FaultyTestResult(Result::MessageWarn, message)));
            }
        } else {
            emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
                tr("Project is null for \"%1\". Removing from test run.\n"
                   "Check the test environment.").arg(config->displayName()))));
        }
    }
    return testCaseCount;
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
            if (auto rc = getRunConfiguration(firstTestCaseTarget(config)))
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

        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn, mssg)));
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
    scheduleNext();
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
    if (!config->project()) {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
            TestRunner::tr("Startup project has changed. Canceling test run."))));
        onFinished();
        return;
    }
    if (!config->hasExecutable()) {
        if (auto *rc = getRunConfiguration(firstTestCaseTarget(config)))
            config->completeTestInformation(rc, TestRunMode::Debug);
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
    ProjectExplorer::Runnable inferior = config->runnable();
    inferior.executable = commandFilePath;

    const QStringList args = config->argumentsForTestRunner(&omitted);
    inferior.commandLineArguments = Utils::QtcProcess::joinArgs(args);
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
    m_futureWatcher.setFuture(futureInterface->future());

    if (useOutputProcessor) {
        TestOutputReader *outputreader = config->outputReader(*futureInterface, nullptr);
        outputreader->setId(inferior.executable);
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
        if (!m_canceled)
            runOrDebugTests();
        else if (m_executingTests)
            onFinished();
    } else {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                                                  tr("Build failed. Canceling test run."))));
        onFinished();
    }
}

void TestRunner::onFinished()
{
    // if we've been canceled and we still have test configurations queued just throw them away
    qDeleteAll(m_selectedTests);
    m_selectedTests.clear();

    disconnect(m_targetConnect);
    m_fakeFutureInterface = nullptr;
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

    QString details = tr("Could not determine which run configuration to choose for running tests");
    if (!testsInfo.isEmpty())
        details.append(QString(" (%1)").arg(testsInfo));
    m_details = new QLabel(details, this);
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
                auto runnable = rc->runnable();
                const QStringList rcDetails = { runnable.executable,
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
