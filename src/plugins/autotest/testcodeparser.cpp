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

#include "autotestconstants.h"
#include "autotest_utils.h"
#include "autotestplugin.h"
#include "testcodeparser.h"
#include "testframeworkmanager.h"
#include "testsettings.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <utils/algorithm.h>
#include <utils/mapreduce.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QDirIterator>
#include <QFuture>
#include <QFutureInterface>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(LOG, "qtc.autotest.testcodeparser")

namespace Autotest {
namespace Internal {

TestCodeParser::TestCodeParser(TestTreeModel *parent)
    : QObject(parent),
      m_model(parent)
{
    // connect to ProgressManager to postpone test parsing when CppModelManager is parsing
    auto progressManager = qobject_cast<Core::ProgressManager *>(Core::ProgressManager::instance());
    connect(progressManager, &Core::ProgressManager::taskStarted,
            this, &TestCodeParser::onTaskStarted);
    connect(progressManager, &Core::ProgressManager::allTasksFinished,
            this, &TestCodeParser::onAllTasksFinished);
    connect(&m_futureWatcher, &QFutureWatcher<TestParseResultPtr>::started,
            this, &TestCodeParser::parsingStarted);
    connect(&m_futureWatcher, &QFutureWatcher<TestParseResultPtr>::finished,
            this, &TestCodeParser::onFinished);
    connect(&m_futureWatcher, &QFutureWatcher<TestParseResultPtr>::resultReadyAt,
            this, [this] (int index) {
        emit testParseResultReady(m_futureWatcher.resultAt(index));
    });
    connect(this, &TestCodeParser::parsingFinished, this, &TestCodeParser::releaseParserInternals);
    m_reparseTimer.setSingleShot(true);
    connect(&m_reparseTimer, &QTimer::timeout, this, &TestCodeParser::parsePostponedFiles);
}

TestCodeParser::~TestCodeParser()
{
}

void TestCodeParser::setState(State state)
{
    if (m_parserState == Shutdown)
        return;
    qCDebug(LOG) << "setState(" << state << "), currentState:" << m_parserState;
    // avoid triggering parse before code model parsing has finished, but mark as dirty
    if (m_codeModelParsing) {
        m_dirty = true;
        qCDebug(LOG) << "Not setting new state - code model parsing is running, just marking dirty";
        return;
    }

    if ((state == Idle) && (m_parserState == PartialParse || m_parserState == FullParse)) {
        qCDebug(LOG) << "Not setting state, parse is running";
        return;
    }
    m_parserState = state;

    if (m_parserState == Idle && ProjectExplorer::SessionManager::startupProject()) {
        if (m_fullUpdatePostponed || m_dirty) {
            emitUpdateTestTree();
        } else if (m_partialUpdatePostponed) {
            m_partialUpdatePostponed = false;
            qCDebug(LOG) << "calling scanForTests with postponed files (setState)";
            if (!m_reparseTimer.isActive())
                scanForTests(m_postponedFiles.toList());
        }
    }
}

void TestCodeParser::syncTestFrameworks(const QVector<Core::Id> &frameworkIds)
{
    if (m_parserState != Idle) {
        // there's a running parse
        m_fullUpdatePostponed = m_partialUpdatePostponed = false;
        m_postponedFiles.clear();
        Core::ProgressManager::instance()->cancelTasks(Constants::TASK_PARSE);
    }
    m_testCodeParsers.clear();
    TestFrameworkManager *frameworkManager = TestFrameworkManager::instance();
    qCDebug(LOG) << "Setting" << frameworkIds << "as current parsers";
    for (const Core::Id &id : frameworkIds) {
        ITestParser *testParser = frameworkManager->testParserForTestFramework(id);
        QTC_ASSERT(testParser, continue);
        m_testCodeParsers.append(testParser);
    }
    updateTestTree();
}

void TestCodeParser::emitUpdateTestTree()
{
    if (m_testCodeParsers.isEmpty())
        return;
    if (m_singleShotScheduled) {
        qCDebug(LOG) << "not scheduling another updateTestTree";
        return;
    }

    qCDebug(LOG) << "adding singleShot";
    m_singleShotScheduled = true;
    QTimer::singleShot(1000, this, &TestCodeParser::updateTestTree);
}

void TestCodeParser::updateTestTree()
{
    m_singleShotScheduled = false;
    if (m_codeModelParsing) {
        m_fullUpdatePostponed = true;
        m_partialUpdatePostponed = false;
        m_postponedFiles.clear();
        return;
    }

    if (!ProjectExplorer::SessionManager::startupProject())
        return;

    m_fullUpdatePostponed = false;
    qCDebug(LOG) << "calling scanForTests (updateTestTree)";
    scanForTests();
}

static QStringList filterFiles(const QString &projectDir, const QStringList &files)
{
    const QSharedPointer<TestSettings> &settings = AutotestPlugin::instance()->settings();
    const QSet<QString> &filters = settings->whiteListFilters.toSet(); // avoid duplicates
    if (!settings->filterScan || filters.isEmpty())
        return files;
    QStringList finalResult;
    for (const QString &file : files) {
        // apply filter only below project directory if file is part of a project
        const QString &fileToProcess = file.startsWith(projectDir)
                ? file.mid(projectDir.size())
                : file;
        for (const QString &filter : filters) {
            if (fileToProcess.contains(filter)) {
                finalResult.push_back(file);
                break;
            }
        }
    }
    return finalResult;
}

// used internally to indicate a parse that failed due to having triggered a parse for a file that
// is not (yet) part of the CppModelManager's snapshot
static bool parsingHasFailed;

/****** threaded parsing stuff *******/

void TestCodeParser::onDocumentUpdated(const QString &fileName)
{
    if (m_codeModelParsing || m_fullUpdatePostponed)
        return;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return;
    if (!project->files(ProjectExplorer::Project::SourceFiles).contains(fileName))
        return;

    scanForTests(QStringList(fileName));
}

void TestCodeParser::onCppDocumentUpdated(const CPlusPlus::Document::Ptr &document)
{
    onDocumentUpdated(document->fileName());
}

void TestCodeParser::onQmlDocumentUpdated(const QmlJS::Document::Ptr &document)
{
    const QString fileName = document->fileName();
    if (!fileName.endsWith(".qbs"))
        onDocumentUpdated(fileName);
}

void TestCodeParser::onStartupProjectChanged(ProjectExplorer::Project *project)
{
    if (m_parserState == FullParse || m_parserState == PartialParse) {
        qCDebug(LOG) << "Canceling scanForTest (startup project changed)";
        Core::ProgressManager::instance()->cancelTasks(Constants::TASK_PARSE);
    }
    emit aboutToPerformFullParse();
    if (project)
        emitUpdateTestTree();
}

void TestCodeParser::onProjectPartsUpdated(ProjectExplorer::Project *project)
{
    if (project != ProjectExplorer::SessionManager::startupProject())
        return;
    if (m_codeModelParsing)
        m_fullUpdatePostponed = true;
    else
        emitUpdateTestTree();
}

void TestCodeParser::aboutToShutdown()
{
    qCDebug(LOG) << "Disabling (immediately) - shutting down";
    State oldState = m_parserState;
    m_parserState = Shutdown;
    if (oldState == PartialParse || oldState == FullParse) {
        m_futureWatcher.cancel();
        m_futureWatcher.waitForFinished();
    }
}

bool TestCodeParser::postponed(const QStringList &fileList)
{
    switch (m_parserState) {
    case Idle:
        if (fileList.size() == 1) {
            if (m_reparseTimerTimedOut)
                return false;
            switch (m_postponedFiles.size()) {
            case 0:
                m_postponedFiles.insert(fileList.first());
                m_reparseTimer.setInterval(1000);
                m_reparseTimer.start();
                return true;
            case 1:
                if (m_postponedFiles.contains(fileList.first())) {
                    m_reparseTimer.start();
                    return true;
                }
                // intentional fall-through
            default:
                m_postponedFiles.insert(fileList.first());
                m_reparseTimer.stop();
                m_reparseTimer.setInterval(0);
                m_reparseTimerTimedOut = false;
                m_reparseTimer.start();
                return true;
            }
        }
        return false;
    case PartialParse:
    case FullParse:
        // parse is running, postponing a full parse
        if (fileList.isEmpty()) {
            m_partialUpdatePostponed = false;
            m_postponedFiles.clear();
            m_fullUpdatePostponed = true;
            qCDebug(LOG) << "Canceling scanForTest (full parse triggered while running a scan)";
            Core::ProgressManager::instance()->cancelTasks(Constants::TASK_PARSE);
        } else {
            // partial parse triggered, but full parse is postponed already, ignoring this
            if (m_fullUpdatePostponed)
                return true;
            // partial parse triggered, postpone or add current files to already postponed partial
            for (const QString &file : fileList)
                m_postponedFiles.insert(file);
            m_partialUpdatePostponed = true;
        }
        return true;
    case Shutdown:
        break;
    }
    QTC_ASSERT(false, return false); // should not happen at all
}

static void parseFileForTests(const QVector<ITestParser *> &parsers,
                              QFutureInterface<TestParseResultPtr> &futureInterface,
                              const QString &fileName)
{
    for (ITestParser *parser : parsers) {
        if (futureInterface.isCanceled())
            return;
        if (parser->processDocument(futureInterface, fileName))
            break;
    }
}

void TestCodeParser::scanForTests(const QStringList &fileList)
{
    if (m_parserState == Shutdown || m_testCodeParsers.isEmpty())
        return;

    if (postponed(fileList))
        return;

    m_reparseTimer.stop();
    m_reparseTimerTimedOut = false;
    m_postponedFiles.clear();
    bool isFullParse = fileList.isEmpty();
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return;
    QStringList list;
    if (isFullParse) {
        list = project->files(ProjectExplorer::Project::SourceFiles);
        if (list.isEmpty()) {
            // at least project file should be there, but might happen if parsing current project
            // takes too long, especially when opening sessions holding multiple projects
            qCDebug(LOG) << "File list empty (FullParse) - trying again in a sec";
            emitUpdateTestTree();
            return;
        }
        qCDebug(LOG) << "setting state to FullParse (scanForTests)";
        m_parserState = FullParse;
    } else {
        list << fileList;
        qCDebug(LOG) << "setting state to PartialParse (scanForTests)";
        m_parserState = PartialParse;
    }

    parsingHasFailed = false;

    if (isFullParse) {
        // remove qml files as they will be found automatically by the referencing cpp file
        list = Utils::filtered(list, [] (const QString &fn) {
            return !fn.endsWith(".qml");
        });
        m_model->markAllForRemoval();
    } else {
        for (const QString &filePath : list)
            m_model->markForRemoval(filePath);
    }

    list = filterFiles(project->projectDirectory().toString(), list);
    if (list.isEmpty()) {
        if (isFullParse) {
            Core::MessageManager::instance()->write(
                        tr("AutoTest Plugin WARNING: No files left after filtering test scan "
                           "folders. Check test filter settings."),
                        Core::MessageManager::Flash);
        }
        onFinished();
        return;
    }
    qCDebug(LOG) << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "StartParsing";
    for (ITestParser *parser : m_testCodeParsers)
        parser->init(list);

    QFuture<TestParseResultPtr> future = Utils::map(list,
        [this](QFutureInterface<TestParseResultPtr> &fi, const QString &file) {
            parseFileForTests(m_testCodeParsers, fi, file);
        },
        Utils::MapReduceOption::Unordered,
        QThread::LowestPriority);
    m_futureWatcher.setFuture(future);
    if (list.size() > 5) {
        Core::ProgressManager::addTask(future, tr("Scanning for Tests"),
                                       Autotest::Constants::TASK_PARSE);
    }
}

void TestCodeParser::onTaskStarted(Core::Id type)
{
    if (type == CppTools::Constants::TASK_INDEX) {
        m_codeModelParsing = true;
        if (m_parserState == FullParse || m_parserState == PartialParse) {
            m_fullUpdatePostponed = m_parserState == FullParse;
            m_partialUpdatePostponed = !m_fullUpdatePostponed;
            qCDebug(LOG) << "Canceling scan for test (CppModelParsing started)";
            parsingHasFailed = true;
            Core::ProgressManager::instance()->cancelTasks(Constants::TASK_PARSE);
        }
    }
}

void TestCodeParser::onAllTasksFinished(Core::Id type)
{
    // if we cancel parsing ensure that progress animation is canceled as well
    if (type == Constants::TASK_PARSE && parsingHasFailed)
        emit parsingFailed();

    // only CPP parsing is relevant as we trigger Qml parsing internally anyway
    if (type != CppTools::Constants::TASK_INDEX)
        return;
    m_codeModelParsing = false;

    // avoid illegal parser state if respective widgets became hidden while parsing
    setState(Idle);
}

void TestCodeParser::onFinished()
{
    if (m_futureWatcher.isCanceled())
        parsingHasFailed = true;
    switch (m_parserState) {
    case PartialParse:
        qCDebug(LOG) << "setting state to Idle (onFinished, PartialParse)";
        m_parserState = Idle;
        onPartialParsingFinished();
        qCDebug(LOG) << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "PartParsingFin";
        break;
    case FullParse:
        qCDebug(LOG) << "setting state to Idle (onFinished, FullParse)";
        m_parserState = Idle;
        m_dirty = parsingHasFailed;
        if (m_partialUpdatePostponed || m_fullUpdatePostponed || parsingHasFailed) {
            onPartialParsingFinished();
        } else {
            qCDebug(LOG) << "emitting parsingFinished"
                         << "(onFinished, FullParse, nothing postponed, parsing succeeded)";
            emit parsingFinished();
            qCDebug(LOG) << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "ParsingFin";
        }
        m_dirty = false;
        break;
    case Shutdown:
        qCDebug(LOG) << "Shutdown complete - not emitting parsingFinished (onFinished)";
        break;
    default:
        qWarning("I should not be here... State: %d", m_parserState);
        break;
    }
}

void TestCodeParser::onPartialParsingFinished()
{
    QTC_ASSERT(m_fullUpdatePostponed != m_partialUpdatePostponed
            || ((m_fullUpdatePostponed || m_partialUpdatePostponed) == false),
               m_partialUpdatePostponed = false;m_postponedFiles.clear(););
    if (m_fullUpdatePostponed) {
        m_fullUpdatePostponed = false;
        qCDebug(LOG) << "calling updateTestTree (onPartialParsingFinished)";
        updateTestTree();
    } else if (m_partialUpdatePostponed) {
        m_partialUpdatePostponed = false;
        qCDebug(LOG) << "calling scanForTests with postponed files (onPartialParsingFinished)";
        if (!m_reparseTimer.isActive())
            scanForTests(m_postponedFiles.toList());
    } else {
        m_dirty |= m_codeModelParsing;
        if (m_dirty) {
            emit parsingFailed();
            qCDebug(LOG) << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "ParsingFail";
        } else if (!m_singleShotScheduled) {
            qCDebug(LOG) << "emitting parsingFinished"
                         << "(onPartialParsingFinished, nothing postponed, not dirty)";
            emit parsingFinished();
            qCDebug(LOG) << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "ParsingFin";
        } else {
            qCDebug(LOG) << "not emitting parsingFinished"
                         << "(on PartialParsingFinished, singleshot scheduled)";
        }
    }
}

void TestCodeParser::parsePostponedFiles()
{
    m_reparseTimerTimedOut = true;
    scanForTests(m_postponedFiles.toList());
}

void TestCodeParser::releaseParserInternals()
{
    for (ITestParser *parser : m_testCodeParsers)
        parser->release();
}

} // namespace Internal
} // namespace Autotest
