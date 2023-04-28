// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltaskmanager.h"
#include "qmljseditorconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/taskhub.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsconstants.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljscheck.h>
#include <utils/async.h>

#include <QDebug>
#include <QtConcurrentRun>

using namespace ProjectExplorer;
using namespace QmlJS;
using namespace Utils;

namespace QmlJSEditor {
namespace Internal {

QmlTaskManager::QmlTaskManager()
{
    // displaying results incrementally leads to flickering
//    connect(&m_messageCollector, &QFutureWatcherBase::resultsReadyAt,
//            this, &QmlTaskManager::displayResults);
    connect(&m_messageCollector, &QFutureWatcherBase::finished,
            this, &QmlTaskManager::displayAllResults);

    m_updateDelay.setInterval(500);
    m_updateDelay.setSingleShot(true);
    connect(&m_updateDelay, &QTimer::timeout, this, [this] { updateMessagesNow(); });
}

static Tasks convertToTasks(const QList<DiagnosticMessage> &messages, const FilePath &fileName, Utils::Id category)
{
    Tasks result;
    for (const DiagnosticMessage &msg : messages) {
        Task::TaskType type = msg.isError() ? Task::Error : Task::Warning;
        Task task(type, msg.message, fileName, msg.loc.startLine, category);
        result += task;
    }
    return result;
}

static Tasks convertToTasks(const QList<StaticAnalysis::Message> &messages, const FilePath &fileName, Utils::Id category)
{
    QList<DiagnosticMessage> diagnostics;
    for (const StaticAnalysis::Message &msg : messages)
        diagnostics += msg.toDiagnosticMessage();
    return convertToTasks(diagnostics, fileName, category);
}

void QmlTaskManager::collectMessages(QPromise<FileErrorMessages> &promise,
                                     Snapshot snapshot,
                                     const QList<ModelManagerInterface::ProjectInfo> &projectInfos,
                                     ViewerContext vContext,
                                     bool updateSemantic)
{
    for (const ModelManagerInterface::ProjectInfo &info : projectInfos) {
        QHash<Utils::FilePath, QList<DiagnosticMessage>> linkMessages;
        ContextPtr context;
        if (updateSemantic) {
            QmlJS::Link link(snapshot, vContext, QmlJS::LibraryInfo());
            context = link(&linkMessages);
        }

        for (const Utils::FilePath &fileName : std::as_const(info.sourceFiles)) {
            Document::Ptr document = snapshot.document(fileName);
            if (!document)
                continue;

            FileErrorMessages result;
            result.fileName = fileName;
            if (document->language().isFullySupportedLanguage()) {
                result.tasks = convertToTasks(document->diagnosticMessages(),
                                              fileName,
                                              Constants::TASK_CATEGORY_QML);

                if (updateSemantic) {
                    result.tasks += convertToTasks(linkMessages.value(fileName),
                                                   fileName,
                                                   Constants::TASK_CATEGORY_QML_ANALYSIS);

                    Check checker(document, context, Core::ICore::settings());
                    result.tasks += convertToTasks(checker(),
                                                   fileName,
                                                   Constants::TASK_CATEGORY_QML_ANALYSIS);
                }
            }

            if (!result.tasks.isEmpty())
                promise.addResult(result);
            if (promise.isCanceled())
                break;
        }
    }
}

void QmlTaskManager::updateMessages()
{
    m_updateDelay.start();
}

void QmlTaskManager::updateSemanticMessagesNow()
{
    updateMessagesNow(true);
}

void QmlTaskManager::updateMessagesNow(bool updateSemantic)
{
    // don't restart a small update if a big one is running
    if (!updateSemantic && m_updatingSemantic)
        return;
    m_updatingSemantic = updateSemantic;

    // abort any update that's going on already
    m_messageCollector.cancel();
    removeAllTasks(updateSemantic);

    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    // process them
    QFuture<FileErrorMessages> future = Utils::asyncRun(
                &collectMessages, modelManager->newestSnapshot(), modelManager->projectInfos(),
                modelManager->defaultVContext(Dialect::AnyLanguage), updateSemantic);
    m_messageCollector.setFuture(future);
}

void QmlTaskManager::documentsRemoved(const Utils::FilePaths &path)
{
    for (const Utils::FilePath &item : path)
        removeTasksForFile(item);
}

void QmlTaskManager::displayResults(int begin, int end)
{
    for (int i = begin; i < end; ++i) {
        const ProjectExplorer::Tasks tasks = m_messageCollector.resultAt(i).tasks;
        for (const Task &task : tasks) {
            insertTask(task);
        }
    }
}

void QmlTaskManager::displayAllResults()
{
    displayResults(0, m_messageCollector.future().resultCount());
    m_updatingSemantic = false;
}

void QmlTaskManager::insertTask(const Task &task)
{
    Tasks tasks = m_docsWithTasks.value(task.file);
    tasks.append(task);
    m_docsWithTasks.insert(task.file, tasks);
    TaskHub::addTask(task);
}

void QmlTaskManager::removeTasksForFile(const Utils::FilePath &fileName)
{
    if (m_docsWithTasks.contains(fileName)) {
        const Tasks tasks = m_docsWithTasks.value(fileName);
        for (const Task &task : tasks)
            TaskHub::removeTask(task);
        m_docsWithTasks.remove(fileName);
    }
}

void QmlTaskManager::removeAllTasks(bool clearSemantic)
{
    TaskHub::clearTasks(Constants::TASK_CATEGORY_QML);
    if (clearSemantic)
        TaskHub::clearTasks(Constants::TASK_CATEGORY_QML_ANALYSIS);
    m_docsWithTasks.clear();
}

} // Internal
} // QmlProjectManager
