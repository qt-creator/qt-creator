// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "taskfile.h"

#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "taskhub.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/session.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QMessageBox>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// TaskFile
// --------------------------------------------------------------------------

QList<TaskFile *> TaskFile::openFiles;

TaskFile::TaskFile(QObject *parent) : Core::IDocument(parent)
{
    setId("TaskList.TaskFile");
}

Core::IDocument::ReloadBehavior TaskFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool TaskFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(flag)

    if (type == TypeRemoved) {
        deleteLater();
        return true;
    }
    return load(errorString, filePath());
}

static Task::TaskType typeFrom(const QString &typeName)
{
    QString tmp = typeName.toLower();
    if (tmp.startsWith("warn"))
        return Task::Warning;
    if (tmp.startsWith("err"))
        return Task::Error;
    return Task::Unknown;
}

static QStringList parseRawLine(const QByteArray &raw)
{
    QStringList result;
    QString line = QString::fromUtf8(raw.constData());
    if (line.startsWith('#'))
        return result;

    return line.split('\t');
}

static QString unescape(const QString &input)
{
    QString result;
    for (int i = 0; i < input.size(); ++i) {
        if (input.at(i) == '\\') {
            if (i == input.size() - 1)
                continue;
            if (input.at(i + 1) == 'n') {
                result.append('\n');
                ++i;
                continue;
            } else if (input.at(i + 1) == 't') {
                result.append('\t');
                ++i;
                continue;
            } else if (input.at(i + 1) == '\\') {
                result.append('\\');
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
        *errorString = Tr::tr("Cannot open task file %1: %2")
                           .arg(name.toUserOutput(), tf.errorString());
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

        if (chunks.size() == 1) {
            description = chunks.at(0);
        } else if (chunks.size() == 2) {
            type = typeFrom(chunks.at(0));
            description = chunks.at(1);
        } else if (chunks.size() == 3) {
            file = chunks.at(0);
            type = typeFrom(chunks.at(1));
            description = chunks.at(2);
        } else if (chunks.size() >= 4) {
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

        if (description.trimmed().isEmpty()) {
            MessageManager::writeFlashing(Tr::tr("Ignoring invalid task (no text)."));
            continue;
        }
        TaskHub::addTask(Task(type, description, FilePath::fromUserInput(file), line,
                              Constants::TASK_CATEGORY_TASKLIST_ID));
    }
    return true;
}

static void clearTasks()
{
    TaskHub::clearTasks(Constants::TASK_CATEGORY_TASKLIST_ID);
}

void TaskFile::stopMonitoring()
{
    SessionManager::setValue(Constants::SESSION_TASKFILE_KEY, {});

    for (TaskFile *document : std::as_const(openFiles))
        document->deleteLater();
    openFiles.clear();
}

bool TaskFile::load(QString *errorString, const FilePath &fileName)
{
    setFilePath(fileName);
    clearTasks();

    bool result = parseTaskFile(errorString, fileName);
    if (result) {
        if (!SessionManager::isDefaultSession(SessionManager::activeSession()))
            SessionManager::setValue(Constants::SESSION_TASKFILE_KEY, fileName.toSettings());
    } else {
        stopMonitoring();
    }

    return result;
}

TaskFile *TaskFile::openTasks(const FilePath &filePath)
{
    TaskFile *file = Utils::findOrDefault(openFiles, Utils::equal(&TaskFile::filePath, filePath));
    QString errorString;
    if (file) {
        file->load(&errorString, filePath);
    } else {
        file = new TaskFile(ProjectExplorerPlugin::instance());

        if (!file->load(&errorString, filePath)) {
            QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("File Error"), errorString);
            delete file;
            return nullptr;
        }

        openFiles.append(file);

        // Register with filemanager:
        Core::DocumentManager::addDocument(file);
    }
    return file;
}

bool StopMonitoringHandler::canHandle(const ProjectExplorer::Task &task) const
{
    return task.category == Constants::TASK_CATEGORY_TASKLIST_ID;
}

void StopMonitoringHandler::handle(const ProjectExplorer::Task &task)
{
    QTC_ASSERT(canHandle(task), return);
    Q_UNUSED(task)
    TaskFile::stopMonitoring();
}

QAction *StopMonitoringHandler::createAction(QObject *parent) const
{
    const QString text = Tr::tr("Stop Monitoring");
    const QString toolTip = Tr::tr("Stop monitoring task files.");
    auto stopMonitoringAction = new QAction(text, parent);
    stopMonitoringAction->setToolTip(toolTip);
    return stopMonitoringAction;
}

} // namespace Internal
} // namespace ProjectExplorer
