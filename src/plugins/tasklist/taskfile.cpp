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

