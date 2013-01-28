/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmltaskmanager.h"
#include "qmljseditorconstants.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/taskhub.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljscheck.h>
#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditoreditable.h>

#include <QDebug>
#include <QtConcurrentRun>
#include <utils/runextensions.h>

using namespace QmlJS;

namespace QmlJSEditor {
namespace Internal {

QmlTaskManager::QmlTaskManager(QObject *parent) :
    QObject(parent),
    m_taskHub(0),
    m_updatingSemantic(false)
{
    m_taskHub = ProjectExplorer::ProjectExplorerPlugin::instance()->taskHub();

    // displaying results incrementally leads to flickering
//    connect(&m_messageCollector, SIGNAL(resultsReadyAt(int,int)),
//            SLOT(displayResults(int,int)));
    connect(&m_messageCollector, SIGNAL(finished()),
            SLOT(displayAllResults()));

    m_updateDelay.setInterval(500);
    m_updateDelay.setSingleShot(true);
    connect(&m_updateDelay, SIGNAL(timeout()),
            SLOT(updateMessagesNow()));
}

static QList<ProjectExplorer::Task> convertToTasks(const QList<DiagnosticMessage> &messages, const Utils::FileName &fileName, const Core::Id &category)
{
    QList<ProjectExplorer::Task> result;
    foreach (const DiagnosticMessage &msg, messages) {
        ProjectExplorer::Task::TaskType type
                = msg.isError() ? ProjectExplorer::Task::Error
                                : ProjectExplorer::Task::Warning;

        ProjectExplorer::Task task(type, msg.message, fileName, msg.loc.startLine,
                                   category);

        result += task;
    }
    return result;
}

static QList<ProjectExplorer::Task> convertToTasks(const QList<StaticAnalysis::Message> &messages, const Utils::FileName &fileName, const Core::Id &category)
{
    QList<DiagnosticMessage> diagnostics;
    foreach (const StaticAnalysis::Message &msg, messages)
        diagnostics += msg.toDiagnosticMessage();
    return convertToTasks(diagnostics, fileName, category);
}

void QmlTaskManager::collectMessages(
        QFutureInterface<FileErrorMessages> &future,
        Snapshot snapshot, QList<ModelManagerInterface::ProjectInfo> projectInfos,
        QStringList importPaths, bool updateSemantic)
{
    foreach (const ModelManagerInterface::ProjectInfo &info, projectInfos) {
        QHash<QString, QList<DiagnosticMessage> > linkMessages;
        ContextPtr context;
        if (updateSemantic) {
            Link link(snapshot, importPaths, snapshot.libraryInfo(info.qtImportsPath));
            context = link(&linkMessages);
        }

        foreach (const QString &fileName, info.sourceFiles) {
            Document::Ptr document = snapshot.document(fileName);
            if (!document)
                continue;

            FileErrorMessages result;
            result.fileName = fileName;
            result.tasks = convertToTasks(document->diagnosticMessages(),
                                          Utils::FileName::fromString(fileName),
                                          Core::Id(Constants::TASK_CATEGORY_QML));

            if (updateSemantic) {
                result.tasks += convertToTasks(linkMessages.value(fileName),
                                               Utils::FileName::fromString(fileName),
                                               Core::Id(Constants::TASK_CATEGORY_QML_ANALYSIS));

                Check checker(document, context);
                result.tasks += convertToTasks(checker(),
                                               Utils::FileName::fromString(fileName),
                                               Core::Id(Constants::TASK_CATEGORY_QML_ANALYSIS));
            }

            if (!result.tasks.isEmpty())
                future.reportResult(result);
            if (future.isCanceled())
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
    QFuture<FileErrorMessages> future =
            QtConcurrent::run<FileErrorMessages>(
                &collectMessages, modelManager->newestSnapshot(), modelManager->projectInfos(),
                modelManager->importPaths(), updateSemantic);
    m_messageCollector.setFuture(future);
}

void QmlTaskManager::documentsRemoved(const QStringList path)
{
    foreach (const QString &item, path)
        removeTasksForFile(item);
}

void QmlTaskManager::displayResults(int begin, int end)
{
    for (int i = begin; i < end; ++i) {
        FileErrorMessages result = m_messageCollector.resultAt(i);
        foreach (const ProjectExplorer::Task &task, result.tasks) {
            insertTask(task);
        }
    }
}

void QmlTaskManager::displayAllResults()
{
    displayResults(0, m_messageCollector.future().resultCount());
    m_updatingSemantic = false;
}

void QmlTaskManager::insertTask(const ProjectExplorer::Task &task)
{
    QList<ProjectExplorer::Task> tasks = m_docsWithTasks.value(task.file.toString());
    tasks.append(task);
    m_docsWithTasks.insert(task.file.toString(), tasks);
    m_taskHub->addTask(task);
}

void QmlTaskManager::removeTasksForFile(const QString &fileName)
{
    if (m_docsWithTasks.contains(fileName)) {
        const QList<ProjectExplorer::Task> tasks = m_docsWithTasks.value(fileName);
        foreach (const ProjectExplorer::Task &task, tasks)
            m_taskHub->removeTask(task);
        m_docsWithTasks.remove(fileName);
    }
}

void QmlTaskManager::removeAllTasks(bool clearSemantic)
{
    m_taskHub->clearTasks(Constants::TASK_CATEGORY_QML);
    if (clearSemantic)
        m_taskHub->clearTasks(Constants::TASK_CATEGORY_QML_ANALYSIS);
    m_docsWithTasks.clear();
}

} // Internal
} // QmlProjectManager
