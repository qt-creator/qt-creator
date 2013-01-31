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

#include "tasklistplugin.h"

#include "stopmonitoringhandler.h"
#include "taskfile.h"
#include "taskfilefactory.h"
#include "tasklistconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <QDir>
#include <QStringList>
#include <QtPlugin>

namespace {

ProjectExplorer::Task::TaskType typeFrom(const QString &typeName)
{
    ProjectExplorer::Task::TaskType type = ProjectExplorer::Task::Unknown;
    QString tmp = typeName.toLower();
    if (tmp.startsWith(QLatin1String("warn")))
        type = ProjectExplorer::Task::Warning;
    else if (tmp.startsWith(QLatin1String("err")))
        type = ProjectExplorer::Task::Error;
    return type;
}

} // namespace

using namespace TaskList;

TaskListPlugin *TaskListPlugin::m_instance = 0;

// --------------------------------------------------------------------------
// TaskListPluginPrivate
// --------------------------------------------------------------------------

class Internal::TaskListPluginPrivate {
public:
    bool parseTaskFile(QString *errorString, ProjectExplorer::Project *context, const QString &name)
    {
        QFile tf(name);
        if (!tf.open(QIODevice::ReadOnly)) {
            *errorString = TaskListPlugin::tr("Cannot open task file %1: %2").arg(
                    QDir::toNativeSeparators(name), tf.errorString());
            return false;
        }

        while (!tf.atEnd())
        {
            QStringList chunks = parseRawLine(tf.readLine());
            if (chunks.isEmpty())
                continue;

            QString description;
            QString file;
            ProjectExplorer::Task::TaskType type = ProjectExplorer::Task::Unknown;
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
                if (fi.isRelative() && context) {
                    QString fullPath = context->projectDirectory() + QLatin1Char('/') + file;
                    fi.setFile(fullPath);
                    file = fi.absoluteFilePath();
                }
            }
            description = unescape(description);

            hub->addTask(ProjectExplorer::Task(type, description,
                                               Utils::FileName::fromUserInput(file), line,
                                               Core::Id(Constants::TASKLISTTASK_ID)));
        }
        return true;
    }

    QStringList parseRawLine(const QByteArray &raw)
    {
        QStringList result;
        QString line = QString::fromUtf8(raw.constData());
        if (line.startsWith(QLatin1Char('#')))
            return result;

        return line.split(QLatin1Char('\t'));
    }

    QString unescape(const QString &input) const
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

    ProjectExplorer::TaskHub *hub;
    TaskFileFactory *fileFactory;
};

// --------------------------------------------------------------------------
// TaskListPlugin
// --------------------------------------------------------------------------

TaskListPlugin::TaskListPlugin() :
    d(new Internal::TaskListPluginPrivate)
{
    m_instance = this;
}

TaskListPlugin::~TaskListPlugin()
{
    delete d;
}

TaskListPlugin *TaskListPlugin::instance()
{
    return m_instance;
}

bool TaskListPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    d->hub = ExtensionSystem::PluginManager::getObject<ProjectExplorer::TaskHub>();

    //: Category under which tasklist tasks are listed in Issues view
    d->hub->addCategory(Core::Id(Constants::TASKLISTTASK_ID), tr("My Tasks"));

    if (!Core::ICore::mimeDatabase()->addMimeTypes(QLatin1String(":tasklist/TaskList.mimetypes.xml"), errorMessage))
        return false;

    d->fileFactory = new Internal::TaskFileFactory(this);
    addAutoReleasedObject(d->fileFactory);
    addAutoReleasedObject(new Internal::StopMonitoringHandler);
    return true;
}

void TaskListPlugin::extensionsInitialized()
{ }

bool TaskListPlugin::loadFile(QString *errorString, ProjectExplorer::Project *context, const QString &fileName)
{
    clearTasks();
    return d->parseTaskFile(errorString, context, fileName);
}

bool TaskListPlugin::monitorFile(ProjectExplorer::Project *context, const QString &fileName)
{
    return d->fileFactory->open(context, fileName);
}

void TaskListPlugin::stopMonitoring()
{
    d->fileFactory->closeAllFiles();
}

void TaskListPlugin::clearTasks()
{
    d->hub->clearTasks(Core::Id(Constants::TASKLISTTASK_ID));
}

Q_EXPORT_PLUGIN(TaskListPlugin)
