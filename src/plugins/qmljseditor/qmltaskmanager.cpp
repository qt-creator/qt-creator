// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltaskmanager.h"
#include "qmljseditorconstants.h"
#include "qmllsclientsettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsconstants.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljscheck.h>
#include <utils/async.h>

#include <QDebug>

using namespace ProjectExplorer;
using namespace QmlJS;
using namespace Utils;

namespace QmlJSEditor::Internal {

QmlTaskManager::QmlTaskManager()
{
    m_updateDelay.setInterval(500);
    m_updateDelay.setSingleShot(true);
    connect(&m_updateDelay, &QTimer::timeout, this, [this] { updateMessagesNow(); });
}

static Tasks convertToTasks(const QList<DiagnosticMessage> &messages, const FilePath &fileName, Id category)
{
    Tasks result;
    for (const DiagnosticMessage &msg : messages) {
        Task::TaskType type = msg.isError() ? Task::Error : Task::Warning;
        Task task(type, msg.message, fileName, msg.loc.startLine, category);
        result += task;
    }
    return result;
}

static Tasks convertToTasks(const QList<StaticAnalysis::Message> &messages, const FilePath &fileName, Id category)
{
    QList<DiagnosticMessage> diagnostics;
    for (const StaticAnalysis::Message &msg : messages)
        diagnostics += msg.toDiagnosticMessage();
    return convertToTasks(diagnostics, fileName, category);
}

void QmlTaskManager::collectMessages(QPromise<FileErrorMessages> &promise,
                                     const Snapshot &snapshot,
                                     const QList<ModelManagerInterface::ProjectInfo> &projectInfos,
                                     const ViewerContext &vContext,
                                     bool updateSemantic)
{
    for (const ModelManagerInterface::ProjectInfo &info : projectInfos) {
        QHash<FilePath, QList<DiagnosticMessage>> linkMessages;
        ContextPtr context;
        if (updateSemantic) {
            QmlJS::Link link(snapshot, vContext, LibraryInfo());
            context = link(&linkMessages);
        }

        for (const FilePath &fileName : std::as_const(info.sourceFiles)) {
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
    // note: this can only be called for the startup project
    BuildSystem *buildSystem = activeBuildSystemForActiveProject();
    if (!buildSystem)
        return;

    const bool isCMake = buildSystem->name() == "cmake";
    // heuristic: qmllint will output meaningful warnings if qmlls is enabled
    if (isCMake && qmllsSettings()->isEnabledOnProject(buildSystem->project())) {
        // abort any update that's going on already, and remove old codemodel warnings
        m_taskTreeRunner.reset();
        removeAllTasks(true);
        buildSystem->buildNamedTarget(Constants::QMLLINT_BUILD_TARGET);
        return;
    }

    updateMessagesNow(true);
}

void QmlTaskManager::updateMessagesNow(bool updateSemantic)
{
    // don't restart a small update if a big one is running
    if (!updateSemantic && m_updatingSemantic)
        return;
    m_updatingSemantic = updateSemantic;

    // abort any update that's going on already
    removeAllTasks(updateSemantic);

    const auto onSetup = [updateSemantic](Async<FileErrorMessages> &task) {
        ModelManagerInterface *modelManager = ModelManagerInterface::instance();
        task.setConcurrentCallData(&collectMessages,
                                   modelManager->newestSnapshot(),
                                   modelManager->projectInfos(),
                                   modelManager->defaultVContext(Dialect::AnyLanguage),
                                   updateSemantic);
    };
    // displaying results incrementally leads to flickering
    const auto onDone = [this](const Async<FileErrorMessages> &task) {
        if (task.isResultAvailable()) {
            const auto results = task.results();
            for (const FileErrorMessages &result : results) {
                const Tasks tasks = result.tasks;
                for (const Task &task : tasks)
                    insertTask(task);
            }
        }
        m_updatingSemantic = false;
    };
    m_taskTreeRunner.start({AsyncTask<FileErrorMessages>(onSetup, onDone)});
}

void QmlTaskManager::documentsRemoved(const FilePaths &paths)
{
    for (const FilePath &path : paths)
        removeTasksForFile(path);
}

void QmlTaskManager::insertTask(const Task &task)
{
    Tasks tasks = m_docsWithTasks.value(task.file());
    tasks.append(task);
    m_docsWithTasks.insert(task.file(), tasks);
    TaskHub::addTask(task);
}

void QmlTaskManager::removeTasksForFile(const FilePath &fileName)
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

} // QmlProjectManager::Internal
