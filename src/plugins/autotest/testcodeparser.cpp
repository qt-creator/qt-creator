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
#include "testcodeparser.h"
#include "testframeworkmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QDirIterator>
#include <QFuture>
#include <QFutureInterface>
#include <QLoggingCategory>
#include <QTimer>

static Q_LOGGING_CATEGORY(LOG, "qtc.autotest.testcodeparser")

namespace Autotest {
namespace Internal {

TestCodeParser::TestCodeParser(TestTreeModel *parent)
    : QObject(parent),
      m_model(parent),
      m_codeModelParsing(false),
      m_fullUpdatePostponed(false),
      m_partialUpdatePostponed(false),
      m_dirty(false),
      m_singleShotScheduled(false),
      m_parserState(Disabled)
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
}

TestCodeParser::~TestCodeParser()
{
}

void TestCodeParser::setState(State state)
{
    qCDebug(LOG) << "setState(" << state << "), currentState:" << m_parserState;
    // avoid triggering parse before code model parsing has finished, but mark as dirty
    if (m_codeModelParsing) {
        m_dirty = true;
        qCDebug(LOG) << "Not setting new state - code model parsing is running, just marking dirty";
        return;
    }

    if ((state == Disabled || state == Idle)
            && (m_parserState == PartialParse || m_parserState == FullParse)) {
        qCDebug(LOG) << "Not setting state, parse is running";
        return;
    }
    m_parserState = state;

    if (m_parserState == Disabled) {
        m_fullUpdatePostponed = m_partialUpdatePostponed = false;
        m_postponedFiles.clear();
    } else if (m_parserState == Idle && ProjectExplorer::SessionManager::startupProject()) {
        if (m_fullUpdatePostponed || m_dirty) {
            emitUpdateTestTree();
        } else if (m_partialUpdatePostponed) {
            m_partialUpdatePostponed = false;
            qCDebug(LOG) << "calling scanForTests with postponed files (setState)";
            scanForTests(m_postponedFiles.toList());
        }
    }
}

void TestCodeParser::syncTestFrameworks(const QVector<Core::Id> &frameworkIds)
{
    if (m_parserState != Disabled && m_parserState != Idle) {
        // there's a running parse
        m_fullUpdatePostponed = m_partialUpdatePostponed = false;
        m_postponedFiles.clear();
        Core::ProgressManager::instance()->cancelTasks(Constants::TASK_PARSE);
    }
    m_testCodeParsers.clear();
    TestFrameworkManager *frameworkManager = TestFrameworkManager::instance();
    qCDebug(LOG) << "Setting" << frameworkIds << "as current parsers";
    foreach (const Core::Id &id, frameworkIds) {
        ITestParser *testParser = frameworkManager->testParserForTestFramework(id);
        QTC_ASSERT(testParser, continue);
        m_testCodeParsers.append(testParser);
    }
    if (m_parserState != Disabled)
        updateTestTree();
}

void TestCodeParser::emitUpdateTestTree()
{
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

    emit aboutToPerformFullParse();
    qCDebug(LOG) << "calling scanForTests (updateTestTree)";
    scanForTests();
}

// used internally to indicate a parse that failed due to having triggered a parse for a file that
// is not (yet) part of the CppModelManager's snapshot
static bool parsingHasFailed;

static bool checkDocumentForTestCode(QFutureInterface<TestParseResultPtr> &futureInterface,
                                     const QString &fileName,
                                     const QVector<ITestParser *> &parsers)
{
    foreach (ITestParser *currentParser, parsers) {
        if (currentParser->processDocument(futureInterface, fileName))
            return true;
    }
    return false;
}

static void performParse(QFutureInterface<TestParseResultPtr> &futureInterface,
                         const QStringList &list, const QVector<ITestParser *> &parsers)
{
    int progressValue = 0;
    futureInterface.setProgressRange(0, list.size());
    futureInterface.setProgressValue(progressValue);

    foreach (const QString &file, list) {
        if (futureInterface.isCanceled())
            return;
        futureInterface.setProgressValue(++progressValue);
        if (!checkDocumentForTestCode(futureInterface, file, parsers)) {
            parsingHasFailed |= !CppTools::CppModelManager::instance()->snapshot().contains(file)
                && (CppTools::ProjectFile::classify(file) != CppTools::ProjectFile::Unclassified);
        }
    }
    futureInterface.setProgressValue(list.size());
}

/****** threaded parsing stuff *******/

void TestCodeParser::onCppDocumentUpdated(const CPlusPlus::Document::Ptr &document)
{
    if (m_codeModelParsing) {
        if (!m_fullUpdatePostponed) {
            m_partialUpdatePostponed = true;
            m_postponedFiles.insert(document->fileName());
        }
        return;
    }

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return;
    const QString fileName = document->fileName();
    if (!project->files(ProjectExplorer::Project::AllFiles).contains(fileName))
        return;

    qCDebug(LOG) << "calling scanForTests (onCppDocumentUpdated)";
    scanForTests(QStringList(fileName));
}

void TestCodeParser::onQmlDocumentUpdated(const QmlJS::Document::Ptr &document)
{
    const QString &fileName = document->fileName();
    if (m_codeModelParsing) {
        if (!m_fullUpdatePostponed) {
            m_partialUpdatePostponed = true;
            m_postponedFiles.insert(fileName);
        }
        return;
    }

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return;
    if (!project->files(ProjectExplorer::Project::AllFiles).contains(fileName)) {
        // what if the file is not listed inside the pro file, but will be used anyway?
        return;
    }

    scanForTests(QStringList(fileName));
}

void TestCodeParser::onStartupProjectChanged(ProjectExplorer::Project *project)
{
    if (m_parserState == FullParse || m_parserState == PartialParse)
        Core::ProgressManager::instance()->cancelTasks(Constants::TASK_PARSE);
    else if (project)
        emitUpdateTestTree();
}

void TestCodeParser::onProjectPartsUpdated(ProjectExplorer::Project *project)
{
    if (project != ProjectExplorer::SessionManager::startupProject())
        return;
    if (m_codeModelParsing || m_parserState == Disabled)
        m_fullUpdatePostponed = true;
    else
        emitUpdateTestTree();
}

bool TestCodeParser::postponed(const QStringList &fileList)
{
    switch (m_parserState) {
    case Idle:
        return false;
    case PartialParse:
    case FullParse:
        // parse is running, postponing a full parse
        if (fileList.isEmpty()) {
            m_partialUpdatePostponed = false;
            m_postponedFiles.clear();
            m_fullUpdatePostponed = true;
            Core::ProgressManager::instance()->cancelTasks(Constants::TASK_PARSE);
        } else {
            // partial parse triggered, but full parse is postponed already, ignoring this
            if (m_fullUpdatePostponed)
                return true;
            // partial parse triggered, postpone or add current files to already postponed partial
            foreach (const QString &file, fileList)
                m_postponedFiles.insert(file);
            m_partialUpdatePostponed = true;
        }
        return true;
    case Disabled:
        break;
    }
    QTC_ASSERT(false, return false); // should not happen at all
}

void TestCodeParser::scanForTests(const QStringList &fileList)
{
    if (m_parserState == Disabled) {
        m_dirty = true;
        if (fileList.isEmpty()) {
            m_fullUpdatePostponed = true;
            m_partialUpdatePostponed = false;
            m_postponedFiles.clear();
        } else if (!m_fullUpdatePostponed) {
            m_partialUpdatePostponed = true;
            foreach (const QString &file, fileList)
                m_postponedFiles.insert(file);
        }
        return;
    }

    if (postponed(fileList))
        return;

    m_postponedFiles.clear();
    bool isFullParse = fileList.isEmpty();
    QStringList list;
    if (isFullParse) {
        list = ProjectExplorer::SessionManager::startupProject()->files(ProjectExplorer::Project::AllFiles);
        if (list.isEmpty())
            return;
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
            return !fn.endsWith(QLatin1String(".qml"));
        });
        m_model->markAllForRemoval();
    } else {
        foreach (const QString &filePath, list)
            m_model->markForRemoval(filePath);
    }

    foreach (ITestParser *parser, m_testCodeParsers)
        parser->init(list);

    QFuture<TestParseResultPtr> future = Utils::runAsync(&performParse, list, m_testCodeParsers);
    m_futureWatcher.setFuture(future);
    if (list.size() > 5) {
        Core::ProgressManager::addTask(future, tr("Scanning for Tests"),
                                       Autotest::Constants::TASK_PARSE);
    }
}

void TestCodeParser::onTaskStarted(Core::Id type)
{
    if (type == CppTools::Constants::TASK_INDEX)
        m_codeModelParsing = true;
}

void TestCodeParser::onAllTasksFinished(Core::Id type)
{
    // only CPP parsing is relevant as we trigger Qml parsing internally anyway
    if (type != CppTools::Constants::TASK_INDEX)
        return;
    m_codeModelParsing = false;

    // avoid illegal parser state if respective widgets became hidden while parsing
    setState(Idle);
}

void TestCodeParser::onFinished()
{
    switch (m_parserState) {
    case PartialParse:
        qCDebug(LOG) << "setting state to Idle (onFinished, PartialParse)";
        m_parserState = Idle;
        onPartialParsingFinished();
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
        }
        m_dirty = false;
        break;
    case Disabled: // can happen if all Test related widgets become hidden while parsing
        qCDebug(LOG) << "emitting parsingFinished (onFinished, Disabled)";
        emit parsingFinished();
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
        scanForTests(m_postponedFiles.toList());
    } else {
        m_dirty |= m_codeModelParsing;
        if (m_dirty) {
            emit parsingFailed();
        } else if (!m_singleShotScheduled) {
            qCDebug(LOG) << "emitting parsingFinished"
                         << "(onPartialParsingFinished, nothing postponed, not dirty)";
            emit parsingFinished();
        } else {
            qCDebug(LOG) << "not emitting parsingFinished"
                         << "(on PartialParsingFinished, singleshot scheduled)";
        }
    }
}

void TestCodeParser::releaseParserInternals()
{
    for (ITestParser *parser : m_testCodeParsers)
        parser->release();
}

} // namespace Internal
} // namespace Autotest
