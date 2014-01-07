/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <QDir>
#include <QStringList>
#include <QtPlugin>

using namespace ProjectExplorer;
using namespace TaskList::Internal;

static const char SESSION_FILE_KEY[] = "TaskList.File";
static const char SESSION_BASE_KEY[] = "TaskList.BaseDir";

namespace TaskList {

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

static bool parseTaskFile(QString *errorString, const QString &base, const QString &name)
{
    QFile tf(name);
    if (!tf.open(QIODevice::ReadOnly)) {
        *errorString = TaskListPlugin::tr("Cannot open task file %1: %2").arg(
                QDir::toNativeSeparators(name), tf.errorString());
        return false;
    }

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
            if (fi.isRelative() && !base.isEmpty()) {
                QString fullPath = base + QLatin1Char('/') + file;
                fi.setFile(fullPath);
                file = fi.absoluteFilePath();
            }
        }
        description = unescape(description);

        TaskHub::addTask(type, description, Constants::TASKLISTTASK_ID,
                         Utils::FileName::fromUserInput(file), line);
    }
    return true;
}

// --------------------------------------------------------------------------
// TaskListPlugin
// --------------------------------------------------------------------------

static TaskFileFactory *m_fileFactory = 0;

bool TaskListPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    //: Category under which tasklist tasks are listed in Issues view
    TaskHub::addCategory(Constants::TASKLISTTASK_ID, tr("My Tasks"));

    if (!Core::MimeDatabase::addMimeTypes(QLatin1String(":tasklist/TaskList.mimetypes.xml"), errorMessage))
        return false;

    m_fileFactory = new TaskFileFactory(this);
    addAutoReleasedObject(m_fileFactory);
    addAutoReleasedObject(new StopMonitoringHandler);

    connect(SessionManager::instance(), SIGNAL(sessionLoaded(QString)),
            this, SLOT(loadDataFromSession()));

    return true;
}

bool TaskListPlugin::loadFile(QString *errorString, const QString &context, const QString &fileName)
{
    clearTasks();

    bool result = parseTaskFile(errorString, context, fileName);
    if (result) {
        SessionManager::setValue(QLatin1String(SESSION_BASE_KEY), context);
        SessionManager::setValue(QLatin1String(SESSION_FILE_KEY), fileName);
    } else {
        stopMonitoring();
    }

    return result;
}

void TaskListPlugin::stopMonitoring()
{
    SessionManager::setValue(QLatin1String(SESSION_BASE_KEY), QString());
    SessionManager::setValue(QLatin1String(SESSION_FILE_KEY), QString());

    m_fileFactory->closeAllFiles();
}

void TaskListPlugin::clearTasks()
{
    TaskHub::clearTasks(Constants::TASKLISTTASK_ID);
}

void TaskListPlugin::loadDataFromSession()
{
    const QString fileName = SessionManager::value(QLatin1String(SESSION_FILE_KEY)).toString();
    if (fileName.isEmpty())
        return;
    m_fileFactory->open(SessionManager::value(QLatin1String(SESSION_BASE_KEY)).toString(), fileName);
}

} // namespace TaskList

Q_EXPORT_PLUGIN(TaskList::TaskListPlugin)
