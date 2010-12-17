/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "taskfile.h"

#include "tasklistplugin.h"

using namespace TaskList;
using namespace TaskList::Internal;

// --------------------------------------------------------------------------
// TaskFile
// --------------------------------------------------------------------------

TaskFile::TaskFile(QObject *parent) : Core::IFile(parent),
    m_context(0)
{ }

TaskFile::~TaskFile()
{ }

bool TaskFile::save(const QString &fileName)
{
    Q_UNUSED(fileName);
    return false;
}

QString TaskFile::fileName() const
{
    return m_fileName;
}

QString TaskFile::defaultPath() const
{
    return QString();
}

QString TaskFile::suggestedFileName() const
{
    return QString();
}

QString TaskFile::mimeType() const
{
    return QString();
}

bool TaskFile::isModified() const
{
    return false;
}

bool TaskFile::isReadOnly() const
{
    return true;
}

bool TaskFile::isSaveAsAllowed() const
{
    return false;
}

Core::IFile::ReloadBehavior TaskFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state);
    if (type != TypePermissions)
        return BehaviorSilent;
    return BehaviorAsk;
}

void TaskFile::reload(ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(flag);

    if (type == TypePermissions)
        return;
    open(m_fileName);
    if (type == TypeRemoved)
        deleteLater();
}

void TaskFile::rename(const QString &newName)
{
    Q_UNUSED(newName);
}

bool TaskFile::open(const QString &fileName)
{
    m_fileName = fileName;
    return TaskList::TaskListPlugin::instance()->loadFile(m_context, m_fileName);
}

ProjectExplorer::Project *TaskFile::context() const
{
    return m_context;
}

void TaskFile::setContext(ProjectExplorer::Project *context)
{
    m_context = context;
}

