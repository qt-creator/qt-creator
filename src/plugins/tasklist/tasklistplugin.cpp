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

#include "tasklistplugin.h"

#include "stopmonitoringhandler.h"
#include "taskfile.h"
#include "tasklistconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocumentfactory.h>
#include <coreplugin/documentmanager.h>
#include <projectexplorer/session.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <QDir>
#include <QMessageBox>
#include <QStringList>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

static const char SESSION_FILE_KEY[] = "TaskList.File";

namespace TaskList {
namespace Internal {

static TaskListPlugin *m_instance;

class TaskListPluginPrivate
{
public:
    QList<TaskFile *> m_openFiles;
    Core::IDocumentFactory m_fileFactory;
    StopMonitoringHandler m_stopMonitoringHandler;
};

static Task::TaskType typeFrom(const QString &typeName)
{
    Task::TaskType type = Task::Unknown;
    QString tmp = typeName.toLower();
    if (tmp.startsWith(QLatin1String("warn")))
        type = Task::Warning;
    else if (tmp.startsWith(QLatin1String("err")))
        type = Task::Error;
    return type;
}

static QStringList parseRawLine(const QByteArray &raw)
{
    QStringList result;
    QString line = QString::fromUtf8(raw.constData());
    if (line.startsWith(QLatin1Char('#')))
        return result;

    return line.split(QLatin1Char('\t'));
}

static QString unescape(const QString &input)
{
    QString result;
    for (int i = 0; i < input.count(); ++i) {
        if (input.at(i) == QLatin1Char('\\')) {
            if (i == input.count() - 1)
                continue;
            if (input.at(i + 1) == QLatin1Char('n')) {
                result.append(QLatin1Char('\n'));
                ++i;
                continue;
            } else if (input.at(i + 1) == QLatin1Char('t')) {
                result.append(QLatin1Char('\t'));
                ++i;
                continue;
            } else if (input.at(i + 1) == QLatin1Char('\\')) {
                result.append(QLatin1Char('\\'));
                ++i;
                continue;
            }
            continue;
        }
        result.append(input.at(i));
    }
    return result;
}

static bool parseTaskFile(QString *errorString, const FilePath &name)
{
    QFile tf(name.toString());
    if (!tf.open(QIODevice::ReadOnly)) {
        *errorString = TaskListPlugin::tr("Cannot open task file %1: %2").arg(
                name.toUserOutput(), tf.errorString());
        return false;
    }

    const FilePath parentDir = name.parentDir();
    while (!tf.atEnd()) {
        QStringList chunks = parseRawLine(tf.readLine());
        if (chunks.isEmpty())
            continue;

        QString description;
        QString file;
        Task::TaskType type = Task::Unknown;
        int line = -1;

        if (chunks.count() == 1) {
            description = chunks.at(0);
        } else if (chunks.count() == 2) {
            type = typeFrom(chunks.at(0));
            description = chunks.at(1);
        } else if (chunks.count() == 3) {
            file = chunks.at(0);
            type = typeFrom(chunks.at(1));
            description = chunks.at(2);
        } else if (chunks.count() >= 4) {
            file = chunks.at(0);
            bool ok;
            line = chunks.at(1).toInt(&ok);
            if (!ok)
                line = -1;
            type = typeFrom(chunks.at(2));
            description = chunks.at(3);
        }
        if (!file.isEmpty()) {
            file = QDir::fromNativeSeparators(file);
            QFileInfo fi(file);
            if (fi.isRelative())
                file = parentDir.pathAppended(file).toString();
        }
        description = unescape(description);

        TaskHub::addTask(Task(type, description, FilePath::fromUserInput(file), line,
                              Constants::TASKLISTTASK_ID));
    }
    return true;
}

// --------------------------------------------------------------------------
// TaskListPlugin
// --------------------------------------------------------------------------

IDocument *TaskListPlugin::openTasks(const FilePath &fileName)
{
    foreach (TaskFile *doc, d->m_openFiles) {
        if (doc->filePath() == fileName)
            return doc;
    }

    auto file = new TaskFile(this);

    QString errorString;
    if (!file->load(&errorString, fileName)) {
        QMessageBox::critical(ICore::dialogParent(), tr("File Error"), errorString);
        delete file;
        return nullptr;
    }

    d->m_openFiles.append(file);

    // Register with filemanager:
    DocumentManager::addDocument(file);

    return file;
}

TaskListPlugin::TaskListPlugin()
{
    m_instance = this;
}

TaskListPlugin::~TaskListPlugin()
{
    delete d;
    m_instance = nullptr;
}

bool TaskListPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    d = new TaskListPluginPrivate;

    //: Category under which tasklist tasks are listed in Issues view
    TaskHub::addCategory(Constants::TASKLISTTASK_ID, tr("My Tasks"));

    d->m_fileFactory.addMimeType(QLatin1String("text/x-tasklist"));
    d->m_fileFactory.setOpener([this](const QString &fileName) {
        return openTasks(FilePath::fromString(fileName));
    });

    connect(SessionManager::instance(), &SessionManager::sessionLoaded,
            this, &TaskListPlugin::loadDataFromSession);

    return true;
}

bool TaskListPlugin::loadFile(QString *errorString, const FilePath &fileName)
{
    clearTasks();

    bool result = parseTaskFile(errorString, fileName);
    if (result) {
        if (!SessionManager::isDefaultSession(SessionManager::activeSession()))
            SessionManager::setValue(QLatin1String(SESSION_FILE_KEY), fileName.toString());
    } else {
        stopMonitoring();
    }

    return result;
}

void TaskListPlugin::stopMonitoring()
{
    SessionManager::setValue(QLatin1String(SESSION_FILE_KEY), QString());

    foreach (TaskFile *document, m_instance->d->m_openFiles)
        document->deleteLater();
    m_instance->d->m_openFiles.clear();
}

void TaskListPlugin::clearTasks()
{
    TaskHub::clearTasks(Constants::TASKLISTTASK_ID);
}

void TaskListPlugin::loadDataFromSession()
{
    const FilePath fileName = FilePath::fromString(
                SessionManager::value(QLatin1String(SESSION_FILE_KEY)).toString());
    if (!fileName.isEmpty())
        openTasks(fileName);
}

} // namespace Internal
} // namespace TaskList
