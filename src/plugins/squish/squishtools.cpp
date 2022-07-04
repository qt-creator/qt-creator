/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "squishtools.h"
#include "squishoutputpane.h"
#include "squishplugin.h"
#include "squishsettings.h"
#include "squishxmloutputhandler.h"

#include <QDebug> // TODO remove

#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QTimer>
#include <QWindow>

namespace Squish {
namespace Internal {

// make this configurable?
static const QString resultsDirectory = QFileInfo(QDir::home(), ".squishQC/Test Results")
                                            .absoluteFilePath();

SquishTools::SquishTools(QObject *parent)
    : QObject(parent)
{
    SquishOutputPane *outputPane = SquishOutputPane::instance();
    connect(this,
            &SquishTools::logOutputReceived,
            outputPane,
            &SquishOutputPane::addLogOutput,
            Qt::QueuedConnection);
    connect(this,
            &SquishTools::squishTestRunStarted,
            outputPane,
            &SquishOutputPane::clearOldResults);
    connect(this,
            &SquishTools::squishTestRunFinished,
            outputPane,
            &SquishOutputPane::onTestRunFinished);
}

SquishTools::~SquishTools()
{
    // TODO add confirmation dialog somewhere
    if (m_runnerProcess) {
        m_runnerProcess->terminate();
        if (!m_runnerProcess->waitForFinished(5000))
            m_runnerProcess->kill();
        delete m_runnerProcess;
        m_runnerProcess = nullptr;
    }

    if (m_serverProcess) {
        m_serverProcess->terminate();
        if (!m_serverProcess->waitForFinished(5000))
            m_serverProcess->kill();
        delete m_serverProcess;
        m_serverProcess = nullptr;
    }

    delete m_xmlOutputHandler;
}

struct SquishToolsSettings
{
    SquishToolsSettings() {}

    QString squishPath;
    QString serverPath = "squishserver";
    QString runnerPath = "squishrunner";
    bool isLocalServer = true;
    bool verboseLog = false;
    QString serverHost = "localhost";
    int serverPort = 9999;
    QString licenseKeyPath;

    // populate members using current settings
    void setup()
    {
        QSharedPointer<SquishSettings> squishSettings = SquishPlugin::instance()->squishSettings();
        squishPath = squishSettings->squishPath.toString();
        serverPath = Utils::HostOsInfo::withExecutableSuffix("squishserver");
        runnerPath = Utils::HostOsInfo::withExecutableSuffix("squishrunner");

        if (!squishPath.isEmpty()) {
            const QDir squishBin(squishPath + QDir::separator() + "bin");
            serverPath = QFileInfo(squishBin, serverPath).absoluteFilePath();
            runnerPath = QFileInfo(squishBin, runnerPath).absoluteFilePath();
        }

        isLocalServer = squishSettings->local;
        serverHost = squishSettings->serverHost;
        serverPort = squishSettings->serverPort;
        verboseLog = squishSettings->verbose;
        licenseKeyPath = squishSettings->licensePath.toString();
    }
};

void SquishTools::runTestCases(const QString &suitePath,
                               const QStringList &testCases,
                               const QStringList &additionalServerArgs,
                               const QStringList &additionalRunnerArgs)
{
    if (m_state != Idle) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              tr("Error"),
                              tr("Squish Tools in unexpected state (%1).\n"
                                 "Refusing to run a test case.")
                                  .arg(m_state));
        return;
    }
    // create test results directory (if necessary) and return on fail
    if (!QDir().mkpath(resultsDirectory)) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              tr("Error"),
                              tr("Could not create test results folder. Canceling test run."));
        return;
    }

    m_suitePath = suitePath;
    m_testCases = testCases;
    m_reportFiles.clear();
    m_additionalServerArguments = additionalServerArgs;

    const QString dateTimeString = QDateTime::currentDateTime().toString("yyyy-MM-ddTHH-mm-ss");
    m_currentResultsDirectory = QFileInfo(QDir(resultsDirectory), dateTimeString).absoluteFilePath();

    m_additionalRunnerArguments = additionalRunnerArgs;
    m_additionalRunnerArguments << "--interactive" << "--resultdir"
                                << QDir::toNativeSeparators(m_currentResultsDirectory);

    delete m_xmlOutputHandler;
    m_xmlOutputHandler = new SquishXmlOutputHandler(this);
    connect(this,
            &SquishTools::resultOutputCreated,
            m_xmlOutputHandler,
            &SquishXmlOutputHandler::outputAvailable,
            Qt::QueuedConnection);

    m_testRunning = true;
    emit squishTestRunStarted();
    startSquishServer(RunTestRequested);
}

void SquishTools::setState(SquishTools::State state)
{
    // TODO check whether state transition is legal
    m_state = state;

    switch (m_state) {
    case Idle:
        m_request = None;
        m_suitePath = QString();
        m_testCases.clear();
        m_reportFiles.clear();
        m_additionalRunnerArguments.clear();
        m_additionalServerArguments.clear();
        m_testRunning = false;
        m_currentResultsDirectory.clear();
        m_lastTopLevelWindows.clear();
        break;
    case ServerStarted:
        if (m_request == RunTestRequested) {
            startSquishRunner();
        } else if (m_request == RecordTestRequested) {
        } else if (m_request == RunnerQueryRequested) {
        } else {
            QTC_ASSERT(false, qDebug() << m_state << m_request);
        }
        break;
    case ServerStartFailed:
        m_state = Idle;
        m_request = None;
        if (m_testRunning) {
            emit squishTestRunFinished();
            m_testRunning = false;
        }
        restoreQtCreatorWindows();
        break;
    case ServerStopped:
        m_state = Idle;
        if (m_request == ServerStopRequested) {
            m_request = None;
            if (m_testRunning) {
                emit squishTestRunFinished();
                m_testRunning = false;
            }
            restoreQtCreatorWindows();
        } else if (m_request == KillOldBeforeRunRunner) {
            startSquishServer(RunTestRequested);
        } else if (m_request == KillOldBeforeRecordRunner) {
            startSquishServer(RecordTestRequested);
        } else if (m_request == KillOldBeforeQueryRunner) {
            startSquishServer(RunnerQueryRequested);
        } else {
            QTC_ASSERT(false, qDebug() << m_state << m_request);
        }
        break;
    case ServerStopFailed:
        if (m_serverProcess && m_serverProcess->state() != QProcess::NotRunning) {
            m_serverProcess->terminate();
            if (!m_serverProcess->waitForFinished(5000)) {
                m_serverProcess->kill();
                delete m_serverProcess;
                m_serverProcess = nullptr;
            }
        }
        m_state = Idle;
        break;
    case RunnerStartFailed:
    case RunnerStopped:
        if (m_testCases.isEmpty()) {
            m_request = ServerStopRequested;
            stopSquishServer();
            QString error;
            SquishXmlOutputHandler::mergeResultFiles(m_reportFiles,
                                                     m_currentResultsDirectory,
                                                     QDir(m_suitePath).dirName(),
                                                     &error);
            if (!error.isEmpty())
                QMessageBox::critical(Core::ICore::dialogParent(), tr("Error"), error);
            logrotateTestResults();
        } else {
            m_xmlOutputHandler->clearForNextRun();
            startSquishRunner();
        }
        break;
    default:
        break;
    }
}

// make sure to execute setup() to populate with current settings before using it
static SquishToolsSettings toolsSettings;

void SquishTools::startSquishServer(Request request)
{
    m_request = request;
    if (m_serverProcess) {
        if (QMessageBox::question(Core::ICore::dialogParent(),
                                  tr("Squish Server Already Running"),
                                  tr("There is still an old Squish server instance running.\n"
                                     "This will cause problems later on.\n\n"
                                     "If you continue, the old instance will be terminated.\n"
                                     "Do you want to continue?"))
            == QMessageBox::Yes) {
            switch (m_request) {
            case RunTestRequested:
                m_request = KillOldBeforeRunRunner;
                break;
            case RecordTestRequested:
                m_request = KillOldBeforeRecordRunner;
                break;
            case RunnerQueryRequested:
                m_request = KillOldBeforeQueryRunner;
                break;
            default:
                QMessageBox::critical(Core::ICore::dialogParent(),
                                      tr("Error"),
                                      tr("Unexpected state or request while starting Squish "
                                         "server. (state: %1, request: %2)")
                                          .arg(m_state)
                                          .arg(m_request));
            }
            stopSquishServer();
        }
        return;
    }

    toolsSettings.setup();
    m_serverPort = -1;

    const Utils::FilePath squishServer = Utils::Environment::systemEnvironment().searchInPath(
        toolsSettings.serverPath);
    if (squishServer.isEmpty()) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              tr("Squish Server Error"),
                              tr("\"%1\" could not be found or is not executable.\n"
                                 "Check the settings.")
                                  .arg(QDir::toNativeSeparators(toolsSettings.serverPath)));
        setState(Idle);
        return;
    }
    toolsSettings.serverPath = squishServer.toString();

    if (true) // TODO squish setting of minimize QC on squish run/record
        minimizeQtCreatorWindows();
    else
        m_lastTopLevelWindows.clear();

    m_serverProcess = new Utils::QtcProcess;
    QStringList arguments;
    // TODO if isLocalServer is false we should start a squishserver on remote device
    if (toolsSettings.isLocalServer)
        arguments << "--local"; // for now - although Squish Docs say "don't use it"
    else
        arguments << "--port" << QString::number(toolsSettings.serverPort);
    if (toolsSettings.verboseLog)
        arguments << "--verbose";

    m_serverProcess->setCommand({Utils::FilePath::fromString(toolsSettings.serverPath), arguments});
    m_serverProcess->setEnvironment(squishEnvironment());

    connect(m_serverProcess, &Utils::QtcProcess::readyReadStandardOutput,
            this, &SquishTools::onServerOutput);
    connect(m_serverProcess, &Utils::QtcProcess::readyReadStandardError,
            this, &SquishTools::onServerErrorOutput);
    connect(m_serverProcess, &Utils::QtcProcess::done,
            this, &SquishTools::onServerFinished);

    setState(ServerStarting);
    m_serverProcess->start();
    if (!m_serverProcess->waitForStarted()) {
        setState(ServerStartFailed);
        qWarning() << "squishserver did not start within 30s";
    }
}

void SquishTools::stopSquishServer()
{
    if (m_serverProcess && m_serverPort > 0) {
        Utils::QtcProcess serverKiller;
        QStringList args;
        args << "--stop" << "--port" << QString::number(m_serverPort);
        serverKiller.setCommand({m_serverProcess->commandLine().executable(), args});
        serverKiller.setEnvironment(m_serverProcess->environment());
        serverKiller.start();
        if (serverKiller.waitForStarted()) {
            if (!serverKiller.waitForFinished()) {
                qWarning() << "Could not shutdown server within 30s";
                setState(ServerStopFailed);
            }
        } else {
            qWarning() << "Could not shutdown server within 30s";
            setState(ServerStopFailed);
        }
    } else {
        qWarning() << "either no process running or port < 1?" << m_serverProcess << m_serverPort;
        setState(ServerStopFailed);
    }
}

void SquishTools::startSquishRunner()
{
    if (!m_serverProcess) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              tr("No Squish Server"),
                              tr("Squish server does not seem to be running.\n"
                                 "(state: %1, request: %2)\n"
                                 "Try again.")
                                  .arg(m_state)
                                  .arg(m_request));
        setState(Idle);
        return;
    }
    if (m_serverPort == -1) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              tr("No Squish Server Port"),
                              tr("Failed to get the server port.\n"
                                 "(state: %1, request: %2)\n"
                                 "Try again.")
                                  .arg(m_state)
                                  .arg(m_request));
        // setting state to ServerStartFailed will terminate/kill the current unusable server
        setState(ServerStartFailed);
        return;
    }

    if (m_runnerProcess) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              tr("Squish Runner Running"),
                              tr("Squish runner seems to be running already.\n"
                                 "(state: %1, request: %2)\n"
                                 "Wait until it has finished and try again.")
                                  .arg(m_state)
                                  .arg(m_request));
        return;
    }

    const Utils::FilePath squishRunner = Utils::Environment::systemEnvironment().searchInPath(
        toolsSettings.runnerPath);
    if (squishRunner.isEmpty()) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              tr("Squish Runner Error"),
                              tr("\"%1\" could not be found or is not executable.\n"
                                 "Check the settings.")
                                  .arg(QDir::toNativeSeparators(toolsSettings.runnerPath)));
        setState(RunnerStopped);
        return;
    }
    toolsSettings.runnerPath = squishRunner.toString();

    m_runnerProcess = new Utils::QtcProcess;

    QStringList args;
    args << m_additionalServerArguments;
    if (!toolsSettings.isLocalServer)
        args << "--host" << toolsSettings.serverHost;
    args << "--port" << QString::number(m_serverPort);
    args << "--debugLog" << "alpw"; // TODO make this configurable?

    const QFileInfo testCasePath(QDir(m_suitePath), m_testCases.takeFirst());
    args << "--testcase" << testCasePath.absoluteFilePath();
    args << "--suitedir" << m_suitePath;

    args << m_additionalRunnerArguments;

    const QString caseReportFilePath = QFileInfo(QString::fromLatin1("%1/%2/%3/results.xml")
                                                     .arg(m_currentResultsDirectory,
                                                          QDir(m_suitePath).dirName(),
                                                          testCasePath.baseName()))
                                           .absoluteFilePath();
    m_reportFiles.append(caseReportFilePath);

    args << "--reportgen"
         << QString::fromLatin1("xml2.2,%1").arg(caseReportFilePath);

    m_runnerProcess->setCommand({Utils::FilePath::fromString(toolsSettings.runnerPath), args});
    m_runnerProcess->setEnvironment(squishEnvironment());

    connect(m_runnerProcess, &Utils::QtcProcess::readyReadStandardError,
            this, &SquishTools::onRunnerErrorOutput);
    connect(m_runnerProcess, &Utils::QtcProcess::done,
            this, &SquishTools::onRunnerFinished);

    setState(RunnerStarting);

    // set up the file system watcher for being able to read the results.xml file
    m_resultsFileWatcher = new QFileSystemWatcher;
    // on second run this directory exists and won't emit changes, so use the current subdirectory
    if (QDir(m_currentResultsDirectory).exists())
        m_resultsFileWatcher->addPath(m_currentResultsDirectory + QDir::separator()
                                      + QDir(m_suitePath).dirName());
    else
        m_resultsFileWatcher->addPath(QFileInfo(m_currentResultsDirectory).absolutePath());

    connect(m_resultsFileWatcher,
            &QFileSystemWatcher::directoryChanged,
            this,
            &SquishTools::onResultsDirChanged);

    m_runnerProcess->start();
    if (!m_runnerProcess->waitForStarted()) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              tr("Squish Runner Error"),
                              tr("Squish runner failed to start within given timeframe."));
        delete m_resultsFileWatcher;
        m_resultsFileWatcher = nullptr;
        setState(RunnerStartFailed);
        return;
    }
    setState(RunnerStarted);
    m_currentResultsXML = new QFile(caseReportFilePath);
}

Utils::Environment SquishTools::squishEnvironment()
{
    Utils::Environment environment = Utils::Environment::systemEnvironment();
    if (!toolsSettings.licenseKeyPath.isEmpty())
        environment.prependOrSet("SQUISH_LICENSEKEY_DIR", toolsSettings.licenseKeyPath);
    environment.prependOrSet("SQUISH_PREFIX", toolsSettings.squishPath);
    return environment;
}

void SquishTools::onServerFinished()
{
    delete m_serverProcess;
    m_serverProcess = nullptr;
    m_serverPort = -1;
    setState(ServerStopped);
}

void SquishTools::onRunnerFinished()
{
    delete m_runnerProcess;
    m_runnerProcess = nullptr;

    if (m_resultsFileWatcher) {
        delete m_resultsFileWatcher;
        m_resultsFileWatcher = nullptr;
    }
    if (m_currentResultsXML) {
        // make sure results are being read if not done yet
        if (m_currentResultsXML->exists() && !m_currentResultsXML->isOpen())
            onResultsDirChanged(m_currentResultsXML->fileName());
        if (m_currentResultsXML->isOpen())
            m_currentResultsXML->close();
        delete m_currentResultsXML;
        m_currentResultsXML = nullptr;
    }
    setState(RunnerStopped);
}

void SquishTools::onServerOutput()
{
    // output used for getting the port information of the current squishserver
    const QByteArray output = m_serverProcess->readAllStandardOutput();
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;
        if (trimmed.startsWith("Port:")) {
            if (m_serverPort == -1) {
                bool ok;
                int port = trimmed.mid(6).toInt(&ok);
                if (ok) {
                    m_serverPort = port;
                    setState(ServerStarted);
                } else {
                    qWarning() << "could not get port number" << trimmed.mid(6);
                    setState(ServerStartFailed);
                }
            } else {
                qWarning() << "got a Port output - don't know why...";
            }
        }
        emit logOutputReceived(QString("Server: ") + QLatin1String(trimmed));
    }
}

void SquishTools::onServerErrorOutput()
{
    // output that must be send to the Runner/Server Log
    const QByteArray output = m_serverProcess->readAllStandardError();
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (!trimmed.isEmpty())
            emit logOutputReceived(QString("Server: ") + QLatin1String(trimmed));
    }
}

static char firstNonWhitespace(const QByteArray &text)
{
    for (int i = 0, limit = text.size(); i < limit; ++i)
        if (isspace(text.at(i)))
            continue;
        else
            return text.at(i);
    return 0;
}

static int positionAfterLastClosingTag(const QByteArray &text)
{
    QList<QByteArray> possibleEndTags;
    possibleEndTags << "</description>"
                    << "</message>"
                    << "</verification>"
                    << "</result>"
                    << "</test>"
                    << "</prolog>"
                    << "</epilog>"
                    << "</SquishReport>";

    int positionStart = text.lastIndexOf("</");
    if (positionStart == -1)
        return -1;

    int positionEnd = text.indexOf('>', positionStart);
    if (positionEnd == -1)
        return -1;

    QByteArray endTag = text.mid(positionStart, positionEnd + 1 - positionStart);
    if (possibleEndTags.contains(endTag))
        return positionEnd + 1;

    return positionAfterLastClosingTag(text.mid(0, positionStart));
}

void SquishTools::onRunnerOutput()
{
    // buffer for already read, but not processed content
    static QByteArray buffer;
    const qint64 currentSize = m_currentResultsXML->size();

    if (currentSize <= m_readResultsCount)
        return;

    QByteArray output = m_currentResultsXML->read(currentSize - m_readResultsCount);
    if (output.isEmpty())
        return;

    if (!buffer.isEmpty())
        output.prepend(buffer);
    // we might read only partial written stuff - so we have to figure out how much we can
    // pass on for further processing and buffer the rest for the next reading
    const int endTag = positionAfterLastClosingTag(output);
    if (endTag < output.size()) {
        buffer = output.mid(endTag);
        output.truncate(endTag);
    } else {
        buffer.clear();
    }

    m_readResultsCount += output.size();

    if (firstNonWhitespace(output) == '<') {
        // output that must be used for the TestResultsPane
        emit resultOutputCreated(output);
    } else {
        const QList<QByteArray> lines = output.split('\n');
        for (const QByteArray &line : lines) {
            const QByteArray trimmed = line.trimmed();
            if (!trimmed.isEmpty())
                emit logOutputReceived("Runner: " + QLatin1String(trimmed));
        }
    }
}

void SquishTools::onRunnerErrorOutput()
{
    // output that must be send to the Runner/Server Log
    const QByteArray output = m_runnerProcess->readAllStandardError();
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (!trimmed.isEmpty())
            emit logOutputReceived("Runner: " + QLatin1String(trimmed));
    }
}

void SquishTools::onResultsDirChanged(const QString &filePath)
{
    if (!m_currentResultsXML)
        return; // runner finished before, m_currentResultsXML deleted

    if (m_currentResultsXML->exists()) {
        delete m_resultsFileWatcher;
        m_resultsFileWatcher = nullptr;
        m_readResultsCount = 0;
        if (m_currentResultsXML->open(QFile::ReadOnly)) {
            m_resultsFileWatcher = new QFileSystemWatcher;
            m_resultsFileWatcher->addPath(m_currentResultsXML->fileName());
            connect(m_resultsFileWatcher,
                    &QFileSystemWatcher::fileChanged,
                    this,
                    &SquishTools::onRunnerOutput);
            // squishrunner might have finished already, call once at least
            onRunnerOutput();
        } else {
            // TODO set a flag to process results.xml as soon the complete test run has finished
            qWarning() << "could not open results.xml although it exists" << filePath
                       << m_currentResultsXML->error() << m_currentResultsXML->errorString();
        }
    } else {
        disconnect(m_resultsFileWatcher);
        // results.xml is created as soon some output has been opened - so try again in a second
        QTimer::singleShot(1000, this, [this, filePath]() { onResultsDirChanged(filePath); });
    }
}

void SquishTools::logrotateTestResults()
{
    // make this configurable?
    const int maxNumberOfTestResults = 10;
    const QStringList existing = QDir(resultsDirectory)
                                     .entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for (int i = 0, limit = existing.size() - maxNumberOfTestResults; i < limit; ++i) {
        QDir current(resultsDirectory + QDir::separator() + existing.at(i));
        if (!current.removeRecursively())
            qWarning() << "could not remove" << current.absolutePath();
    }
}

void SquishTools::minimizeQtCreatorWindows()
{
    m_lastTopLevelWindows = QApplication::topLevelWindows();
    QWindowList toBeRemoved;
    for (QWindow *window : qAsConst(m_lastTopLevelWindows)) {
        if (window->isVisible())
            window->showMinimized();
        else
            toBeRemoved.append(window);
    }

    for (QWindow *window : qAsConst(toBeRemoved))
        m_lastTopLevelWindows.removeOne(window);
}

void SquishTools::restoreQtCreatorWindows()
{
    for (QWindow *window : qAsConst(m_lastTopLevelWindows)) {
        window->requestActivate();
        window->showNormal();
    }
}

} // namespace Internal
} // namespace Squish
