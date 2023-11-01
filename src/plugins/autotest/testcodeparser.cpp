// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testcodeparser.h"

#include "autotestconstants.h"
#include "autotesttr.h"
#include "testsettings.h"
#include "testtreemodel.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/taskprogress.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppmodelmanager.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>

using namespace Core;
using namespace Utils;

namespace Autotest {
namespace Internal {

Q_LOGGING_CATEGORY(LOG, "qtc.autotest.testcodeparser", QtWarningMsg)

using namespace ProjectExplorer;

static bool isProjectParsing()
{
    const BuildSystem *bs = ProjectManager::startupBuildSystem();
    return bs && (bs->isParsing() || bs->isWaitingForParse());
}

TestCodeParser::TestCodeParser()
{
    // connect to ProgressManager to postpone test parsing when CppModelManager is parsing
    ProgressManager *progressManager = ProgressManager::instance();
    connect(progressManager, &ProgressManager::taskStarted,
            this, &TestCodeParser::onTaskStarted);
    connect(progressManager, &ProgressManager::allTasksFinished,
            this, &TestCodeParser::onAllTasksFinished);
    connect(this, &TestCodeParser::parsingFinished, this, &TestCodeParser::releaseParserInternals);
    connect(EditorManager::instance(), &EditorManager::documentClosed, this, [this](IDocument *doc){
        QTC_ASSERT(doc, return);
        if (FilePath filePath = doc->filePath(); filePath.endsWith(".qml"))
            m_qmlEditorRev.remove(filePath);
    });
    m_reparseTimer.setSingleShot(true);
    connect(&m_reparseTimer, &QTimer::timeout, this, &TestCodeParser::parsePostponedFiles);
}

TestCodeParser::~TestCodeParser() = default;

void TestCodeParser::setState(State state)
{
    if (m_parserState == Shutdown)
        return;
    qCDebug(LOG) << "setState(" << state << "), currentState:" << m_parserState;
    if (m_parserState == DisabledTemporarily && state == Idle) {
        m_parserState = Idle;
        qCDebug(LOG) << "Just re-enabling parser.";
        return;
    }

    // avoid triggering parse before code model parsing has finished, but mark as dirty
    if (isProjectParsing() || m_codeModelParsing) {
        m_dirty = true;
        qCDebug(LOG) << "Not setting new state - code model parsing is running, just marking dirty";
        return;
    }

    if ((state == Idle) && (m_parserState == PartialParse || m_parserState == FullParse)) {
        qCDebug(LOG) << "Not setting state, parse is running";
        return;
    }
    m_parserState = state;

    if (m_parserState == Idle && ProjectManager::startupProject()) {
        if (m_postponedUpdateType == UpdateType::FullUpdate || m_dirty) {
            emitUpdateTestTree();
        } else if (m_postponedUpdateType == UpdateType::PartialUpdate) {
            m_postponedUpdateType = UpdateType::NoUpdate;
            qCDebug(LOG) << "calling scanForTests with postponed files (setState)";
            if (!m_reparseTimer.isActive())
                scanForTests(m_postponedFiles);
        }
    }
}

void TestCodeParser::syncTestFrameworks(const QList<ITestParser *> &parsers)
{
    if (m_parserState != Idle) {
        // there's a running parse
        m_postponedUpdateType = UpdateType::FullUpdate;
        m_postponedFiles.clear();
        ProgressManager::cancelTasks(Constants::TASK_PARSE);
    }
    qCDebug(LOG) << "Setting" << parsers << "as current parsers";
    m_testCodeParsers = parsers;
}

void TestCodeParser::emitUpdateTestTree(ITestParser *parser)
{
    if (m_testCodeParsers.isEmpty())
        return;
    if (parser)
        m_updateParsers.insert(parser);
    else
        m_updateParsers.clear();
    if (m_singleShotScheduled) {
        qCDebug(LOG) << "not scheduling another updateTestTree";
        return;
    }

    qCDebug(LOG) << "adding singleShot";
    m_singleShotScheduled = true;
    QTimer::singleShot(1000, this, [this] { updateTestTree(m_updateParsers); });
}

void TestCodeParser::updateTestTree(const QSet<ITestParser *> &parsers)
{
    m_singleShotScheduled = false;
    if (isProjectParsing() || m_codeModelParsing) {
        m_postponedUpdateType = UpdateType::FullUpdate;
        m_postponedFiles.clear();
        if (parsers.isEmpty()) {
            m_updateParsers.clear();
        } else {
            for (ITestParser *parser : parsers)
                m_updateParsers.insert(parser);
        }
        return;
    }

    if (!ProjectManager::startupProject())
        return;

    m_postponedUpdateType = UpdateType::NoUpdate;
    qCDebug(LOG) << "calling scanForTests (updateTestTree)";
    const QList<ITestParser *> sortedParsers = Utils::sorted(Utils::toList(parsers),
                [](const ITestParser *lhs, const ITestParser *rhs) {
        return lhs->framework()->priority() < rhs->framework()->priority();
    });
    scanForTests({}, sortedParsers);
}

/****** threaded parsing stuff *******/

void TestCodeParser::onDocumentUpdated(const FilePath &fileName, bool isQmlFile)
{
    if (isProjectParsing() || m_codeModelParsing || m_postponedUpdateType == UpdateType::FullUpdate)
        return;

    Project *project = ProjectManager::startupProject();
    if (!project)
        return;
    // Quick tests: qml files aren't necessarily listed inside project files
    if (!isQmlFile && !project->isKnownFile(fileName))
        return;

    scanForTests({fileName});
}

void TestCodeParser::onCppDocumentUpdated(const CPlusPlus::Document::Ptr &document)
{
    onDocumentUpdated(document->filePath());
}

void TestCodeParser::onQmlDocumentUpdated(const QmlJS::Document::Ptr &document)
{
    static const QStringList ignoredSuffixes{ "qbs", "ui.qml" };
    const FilePath fileName = document->fileName();
    int editorRevision = document->editorRevision();
    if (editorRevision != m_qmlEditorRev.value(fileName, 0)) {
        m_qmlEditorRev.insert(fileName, editorRevision);
        if (!ignoredSuffixes.contains(fileName.suffix()))
            onDocumentUpdated(fileName, true);
    }
}

void TestCodeParser::onStartupProjectChanged(Project *project)
{
    m_qmlEditorRev.clear();
    if (m_parserState == FullParse || m_parserState == PartialParse) {
        qCDebug(LOG) << "Canceling scanForTest (startup project changed)";
        ProgressManager::cancelTasks(Constants::TASK_PARSE);
    }
    emit aboutToPerformFullParse();
    if (project)
        emitUpdateTestTree();
}

void TestCodeParser::onProjectPartsUpdated(Project *project)
{
    if (project != ProjectManager::startupProject())
        return;
    if (isProjectParsing() || m_codeModelParsing)
        m_postponedUpdateType = UpdateType::FullUpdate;
    else
        emitUpdateTestTree();
}

void TestCodeParser::aboutToShutdown(bool isFinal)
{
    qCDebug(LOG) << "Disabling (immediately) -"
                 << (isFinal ? "shutting down" : "disabled temporarily");
    m_parserState = isFinal ? Shutdown : DisabledTemporarily;
    m_taskTree.reset();
    m_futureSynchronizer.waitForFinished();
    if (!isFinal)
        onFinished(false);
}

bool TestCodeParser::postponed(const QSet<FilePath> &filePaths)
{
    switch (m_parserState) {
    case Idle:
        if (filePaths.size() == 1) {
            if (m_reparseTimerTimedOut)
                return false;
            const FilePath filePath = *filePaths.begin();
            switch (m_postponedFiles.size()) {
            case 0:
                m_postponedFiles.insert(filePath);
                m_reparseTimer.setInterval(1000);
                m_reparseTimer.start();
                return true;
            case 1:
                if (m_postponedFiles.contains(filePath)) {
                    m_reparseTimer.start();
                    return true;
                }
                Q_FALLTHROUGH();
            default:
                m_postponedFiles.insert(filePath);
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
        if (filePaths.isEmpty()) {
            m_postponedFiles.clear();
            m_postponedUpdateType = UpdateType::FullUpdate;
            qCDebug(LOG) << "Canceling scanForTest (full parse triggered while running a scan)";
            ProgressManager::cancelTasks(Constants::TASK_PARSE);
        } else {
            // partial parse triggered, but full parse is postponed already, ignoring this
            if (m_postponedUpdateType == UpdateType::FullUpdate)
                return true;
            // partial parse triggered, postpone or add current files to already postponed partial
            m_postponedFiles += filePaths;
            m_postponedUpdateType = UpdateType::PartialUpdate;
        }
        return true;
    case Shutdown:
    case DisabledTemporarily:
        break;
    }
    QTC_ASSERT(false, return false); // should not happen at all
}

static void parseFileForTests(QPromise<TestParseResultPtr> &promise,
                              const QList<ITestParser *> &parsers, const FilePath &fileName)
{
    for (ITestParser *parser : parsers) {
        if (promise.isCanceled())
            return;
        if (parser->processDocument(promise, fileName))
            break;
    }
}

void TestCodeParser::scanForTests(const QSet<FilePath> &filePaths,
                                  const QList<ITestParser *> &parsers)
{
    if (m_parserState == Shutdown || m_parserState == DisabledTemporarily || m_testCodeParsers.isEmpty())
        return;

    if (postponed(filePaths))
        return;

    QSet<FilePath> files = filePaths; // avoid getting cleared if m_postponedFiles have been passed
    m_reparseTimer.stop();
    m_reparseTimerTimedOut = false;
    m_postponedFiles.clear();
    const bool isFullParse = files.isEmpty();
    Project *project = ProjectManager::startupProject();
    if (!project)
        return;
    if (isFullParse) {
        files = Utils::toSet(project->files(Project::SourceFiles));
        if (files.isEmpty()) {
            // at least project file should be there, but might happen if parsing current project
            // takes too long, especially when opening sessions holding multiple projects
            qCDebug(LOG) << "File list empty (FullParse) - trying again in a sec";
            emitUpdateTestTree();
            return;
        } else if (files.size() == 1 && *files.constBegin() == project->projectFilePath()) {
            qCDebug(LOG) << "File list contains only the project file.";
            return;
        }

        qCDebug(LOG) << "setting state to FullParse (scanForTests)";
        m_parserState = FullParse;
    } else {
        qCDebug(LOG) << "setting state to PartialParse (scanForTests)";
        m_parserState = PartialParse;
    }

    m_parsingHasFailed = false;
    TestTreeModel::instance()->updateCheckStateCache();
    if (isFullParse) {
        // remove qml files as they will be found automatically by the referencing cpp file
        files = Utils::filtered(files, [](const FilePath &fn) { return !fn.endsWith(".qml"); });
        if (!parsers.isEmpty()) {
            for (ITestParser *parser : parsers)
                parser->framework()->rootNode()->markForRemovalRecursively(true);
        } else {
            emit requestRemoveAllFrameworkItems();
        }
    } else if (!parsers.isEmpty()) {
        for (ITestParser *parser: parsers) {
            parser->framework()->rootNode()->markForRemovalRecursively(files);
        }
    } else {
        emit requestRemoval(files);
    }

    QTC_ASSERT(!(isFullParse && files.isEmpty()), onFinished(true); return);

    // use only a single parser or all current active?
    const QList<ITestParser *> codeParsers = parsers.isEmpty() ? m_testCodeParsers : parsers;
    qCDebug(LOG) << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "StartParsing";
    m_parsingTimer.restart();
    QSet<QString> extensions;
    const auto cppSnapshot = CppEditor::CppModelManager::snapshot();

    for (ITestParser *parser : codeParsers) {
        parser->init(files, isFullParse);
        for (const QString &ext : parser->supportedExtensions())
            extensions.insert(ext);
    }
    // We are only interested in files that have been either parsed by the c++ parser,
    // or have an extension that one of the parsers is specifically interested in.
    const QSet<FilePath> filteredFiles
        = Utils::filtered(files, [&extensions, &cppSnapshot](const FilePath &fn) {
              const bool isSupportedExtension = Utils::anyOf(extensions, [&fn](const QString &ext) {
                  return fn.suffix() == ext;
              });
              if (isSupportedExtension)
                  return true;
              return cppSnapshot.contains(fn);
          });

    qCDebug(LOG) << "Starting scan of" << filteredFiles.size() << "(" << files.size() << ")"
                 << "files with" << codeParsers.size() << "parsers";

    using namespace Tasking;

    int limit = testSettings().scanThreadLimit();
    if (limit == 0)
        limit = std::max(QThread::idealThreadCount() / 4, 1);
    qCDebug(LOG) << "Using" << limit << "threads for scan.";
    QList<GroupItem> tasks{parallelLimit(limit)};
    for (const FilePath &file : filteredFiles) {
        const auto setup = [this, codeParsers, file](Async<TestParseResultPtr> &async) {
            async.setConcurrentCallData(parseFileForTests, codeParsers, file);
            async.setPriority(QThread::LowestPriority);
            async.setFutureSynchronizer(&m_futureSynchronizer);
        };
        const auto onDone = [this](const Async<TestParseResultPtr> &async) {
            const QList<TestParseResultPtr> results = async.results();
            for (const TestParseResultPtr &result : results)
                emit testParseResultReady(result);
        };
        tasks.append(AsyncTask<TestParseResultPtr>(setup, onDone));
    }
    m_taskTree.reset(new TaskTree{tasks});
    const auto onDone = [this] { m_taskTree.release()->deleteLater(); onFinished(true); };
    const auto onError = [this] { m_taskTree.release()->deleteLater(); onFinished(false); };
    connect(m_taskTree.get(), &TaskTree::started, this, &TestCodeParser::parsingStarted);
    connect(m_taskTree.get(), &TaskTree::done, this, onDone);
    connect(m_taskTree.get(), &TaskTree::errorOccurred, this, onError);
    if (filteredFiles.size() > 5) {
        auto progress = new TaskProgress(m_taskTree.get());
        progress->setDisplayName(Tr::tr("Scanning for Tests"));
        progress->setId(Constants::TASK_PARSE);
    }
    m_taskTree->start();
}

void TestCodeParser::onTaskStarted(Id type)
{
    if (type == CppEditor::Constants::TASK_INDEX) {
        m_codeModelParsing = true;
        if (m_parserState == FullParse || m_parserState == PartialParse) {
            m_postponedUpdateType = m_parserState == FullParse
                    ? UpdateType::FullUpdate : UpdateType::PartialUpdate;
            qCDebug(LOG) << "Canceling scan for test (CppModelParsing started)";
            m_parsingHasFailed = true;
            ProgressManager::cancelTasks(Constants::TASK_PARSE);
        }
    }
}

void TestCodeParser::onAllTasksFinished(Id type)
{
    // if we cancel parsing ensure that progress animation is canceled as well
    if (type == Constants::TASK_PARSE && m_parsingHasFailed)
        emit parsingFailed();

    // only CPP parsing is relevant as we trigger Qml parsing internally anyway
    if (type != CppEditor::Constants::TASK_INDEX)
        return;
    m_codeModelParsing = false;

    // avoid illegal parser state if respective widgets became hidden while parsing
    if (m_parserState != DisabledTemporarily)
        setState(Idle);
}

void TestCodeParser::onFinished(bool success)
{
    m_parsingHasFailed = !success;
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
        m_dirty = m_parsingHasFailed;
        if (m_postponedUpdateType != UpdateType::NoUpdate || m_parsingHasFailed) {
            onPartialParsingFinished();
        } else {
            qCDebug(LOG) << "emitting parsingFinished"
                         << "(onFinished, FullParse, nothing postponed, parsing succeeded)";
            m_updateParsers.clear();
            emit parsingFinished();
            qCDebug(LOG) << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "ParsingFin";
            qCDebug(LOG) << "Parsing took:" << m_parsingTimer.elapsed() << "ms";
            if (LOG().isInfoEnabled()) {
                qCInfo(LOG).noquote().nospace()
                        << "Current test tree:" << TestTreeModel::instance()->report(true);
            } else {
                qCDebug(LOG).noquote().nospace()
                        << "Current test tree:" << TestTreeModel::instance()->report(false);
            }
        }
        m_dirty = false;
        break;
    case Shutdown:
        qCDebug(LOG) << "Shutdown complete - not emitting parsingFinished (onFinished)";
        break;
    case DisabledTemporarily:
        qCDebug(LOG) << "Disabling complete - emitting parsingFinished";
        emit parsingFinished(); // ensure hidden progress indicator
        break;
    default:
        qWarning("I should not be here... State: %d", m_parserState);
        break;
    }
}

void TestCodeParser::onPartialParsingFinished()
{
    const UpdateType oldType = m_postponedUpdateType;
    m_postponedUpdateType = UpdateType::NoUpdate;
    switch (oldType) {
    case UpdateType::FullUpdate:
        qCDebug(LOG) << "calling updateTestTree (onPartialParsingFinished)";
        updateTestTree(m_updateParsers);
        break;
    case UpdateType::PartialUpdate:
        qCDebug(LOG) << "calling scanForTests with postponed files (onPartialParsingFinished)";
        if (!m_reparseTimer.isActive())
            scanForTests(m_postponedFiles);
        break;
    case UpdateType::NoUpdate:
        m_dirty |= m_codeModelParsing;
        if (m_dirty) {
            emit parsingFailed();
            qCDebug(LOG) << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "ParsingFail";
        } else if (!m_singleShotScheduled) {
            qCDebug(LOG) << "emitting parsingFinished"
                         << "(onPartialParsingFinished, nothing postponed, not dirty)";
            m_updateParsers.clear();
            emit parsingFinished();
            qCDebug(LOG) << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "ParsingFin";
            if (LOG().isDebugEnabled()) {
                QMetaObject::invokeMethod(this, [] { // sweep() needs to be processed before logging
                    qCDebug(LOG).noquote().nospace()
                            << "Current test tree:" << TestTreeModel::instance()->report(false);
                }, Qt::QueuedConnection);
            }
        } else {
            qCDebug(LOG) << "not emitting parsingFinished"
                         << "(on PartialParsingFinished, singleshot scheduled)";
        }
        break;
    }
}

void TestCodeParser::parsePostponedFiles()
{
    m_reparseTimerTimedOut = true;
    scanForTests(m_postponedFiles);
}

void TestCodeParser::releaseParserInternals()
{
    for (ITestParser *parser : std::as_const(m_testCodeParsers))
        parser->release();
}

} // namespace Internal
} // namespace Autotest
