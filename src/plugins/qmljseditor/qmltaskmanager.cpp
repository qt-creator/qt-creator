/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmltaskmanager.h"
#include "qmljseditorconstants.h"

#include <coreplugin/ifile.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/taskhub.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljscheck.h>
#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditoreditable.h>

#include <QtCore/QDebug>
#include <QtCore/QtConcurrentRun>
#include <qtconcurrent/runextensions.h>

using namespace QmlJS;

namespace QmlJSEditor {
namespace Internal {

QmlTaskManager::QmlTaskManager(QObject *parent) :
        QObject(parent),
        m_taskHub(0)
{
    m_taskHub = ExtensionSystem::PluginManager::instance()->getObject<ProjectExplorer::TaskHub>();
    // displaying results incrementally leads to flickering
//    connect(&m_messageCollector, SIGNAL(resultsReadyAt(int,int)),
//            SLOT(displayResults(int,int)));
    connect(&m_messageCollector, SIGNAL(finished()),
            SLOT(displayAllResults()));

    m_updateDelay.setInterval(100);
    m_updateDelay.setSingleShot(true);
    connect(&m_updateDelay, SIGNAL(timeout()),
            SLOT(updateMessagesNow()));
}

void QmlTaskManager::collectMessages(QFutureInterface<FileErrorMessages> &future,
    Snapshot snapshot, QStringList files, QStringList /*importPaths*/)
{
    // ### link and check error messages are disabled for now: too many false-positives!
    //Interpreter::Context ctx(snapshot);
    //QHash<QString, QList<DiagnosticMessage> > linkMessages;
    //Link link(&ctx, snapshot, importPaths);
    //link(&linkMessages);

    foreach (const QString &fileName, files) {
        Document::Ptr document = snapshot.document(fileName);
        if (!document)
            continue;

        FileErrorMessages result;
        result.fileName = fileName;
        result.messages = document->diagnosticMessages();

        //result.messages += linkMessages.value(fileName);

        //Check checker(document, &ctx);
        //result.messages.append(checker());

        future.reportResult(result);
        if (future.isCanceled())
            break;
    }
}

void QmlTaskManager::updateMessages()
{
    m_updateDelay.start();
}

void QmlTaskManager::updateMessagesNow()
{
    // abort any update that's going on already
    m_messageCollector.cancel();
    removeAllTasks();

    // collect all the source files in open projects
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();
    QStringList sourceFiles;
    foreach (const ModelManagerInterface::ProjectInfo &info, modelManager->projectInfos()) {
        sourceFiles += info.sourceFiles;
    }

    // process them
    QFuture<FileErrorMessages> future =
            QtConcurrent::run<FileErrorMessages>(
                &collectMessages, modelManager->snapshot(), sourceFiles,
                modelManager->importPaths());
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
        foreach (const DiagnosticMessage &msg, result.messages) {
            ProjectExplorer::Task::TaskType type
                    = msg.isError() ? ProjectExplorer::Task::Error
                                    : ProjectExplorer::Task::Warning;

            ProjectExplorer::Task task(type, msg.message, result.fileName, msg.loc.startLine,
                                       Constants::TASK_CATEGORY_QML);

            insertTask(task);
        }
    }
}

void QmlTaskManager::displayAllResults()
{
    displayResults(0, m_messageCollector.future().resultCount());
}

void QmlTaskManager::insertTask(const ProjectExplorer::Task &task)
{
    QList<ProjectExplorer::Task> tasks = m_docsWithTasks.value(task.file);
    tasks.append(task);
    m_docsWithTasks.insert(task.file, tasks);
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

void QmlTaskManager::removeAllTasks()
{
    QMapIterator<QString, QList<ProjectExplorer::Task> > it(m_docsWithTasks);
    while (it.hasNext()) {
        it.next();
        foreach (const ProjectExplorer::Task &task, it.value())
            m_taskHub->removeTask(task);
    }
    m_docsWithTasks.clear();
}

} // Internal
} // QmlProjectManager
