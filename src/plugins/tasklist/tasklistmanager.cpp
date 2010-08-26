/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "tasklistmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/taskhub.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>

namespace {
const char * const TASKLIST_ID = "TaskList.ListId";

ProjectExplorer::Task::TaskType typeFrom(const QString &taskType)
{
    QString type = taskType.toLower();
    if (type.startsWith("warn"))
        return ProjectExplorer::Task::Warning;
    else if (type.startsWith("err"))
        return ProjectExplorer::Task::Error;
    return ProjectExplorer::Task::Unknown;
}

} // namespace

using namespace TaskList::Internal;

TaskListManager::TaskListManager(QObject *parent) :
    QObject(parent),
    m_hub(0)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    m_hub = pm->getObject<ProjectExplorer::TaskHub>();
    m_hub->addCategory(QLatin1String(TASKLIST_ID), tr("My tasks"));
}

TaskListManager::~TaskListManager()
{ }

void TaskListManager::setTaskFile(const QString &name)
{
    m_hub->clearTasks(QLatin1String(TASKLIST_ID));
    parseTaskFile(name);
}

void TaskListManager::parseTaskFile(const QString &name)
{
    QFile tf(name);
    if (!tf.open(QIODevice::ReadOnly))
        return;
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
        m_hub->addTask(ProjectExplorer::Task(type, description, file, line, QLatin1String(TASKLIST_ID)));
    }
}

QStringList TaskListManager::parseRawLine(const QByteArray &raw)
{
    QStringList result;
    QString line = QString::fromUtf8(raw.constData());
    if (line.startsWith(QChar('#')))
        return result;

    result = line.split(QChar('\t'));
    for (int i = 0; i < result.count(); ++i)
        result[i] = unescape(result.at(i));

    return result;
}

QString TaskListManager::unescape(const QString &input) const
{
    QString result;
    for (int i = 0; i < input.count(); ++i) {
        if (input.at(i) == QChar('\\')) {
            if (i == input.count() - 1)
                continue;
            if (input.at(i + 1) == QChar('n')) {
                result.append(QChar('\n'));
                ++i;
                continue;
            } else if (input.at(i + 1) == QChar('t')) {
                result.append(QChar('\t'));
                ++i;
                continue;
            } else if (input.at(i + 1) == QChar('\\')) {
                result.append(QChar('\\'));
                ++i;
                continue;
            }
            continue;
        }
        result.append(input.at(i));
    }
    return result;
}
