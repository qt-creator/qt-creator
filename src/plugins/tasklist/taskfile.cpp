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

#include "taskfile.h"

#include "tasklistplugin.h"

using namespace TaskList;
using namespace TaskList::Internal;

// --------------------------------------------------------------------------
// TaskFile
// --------------------------------------------------------------------------

TaskFile::TaskFile(QObject *parent) : Core::IDocument(parent),
    m_context(0)
{ }

TaskFile::~TaskFile()
{ }

bool TaskFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(fileName);
    Q_UNUSED(autoSave)
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

bool TaskFile::isSaveAsAllowed() const
{
    return false;
}

Core::IDocument::ReloadBehavior TaskFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state);
    Q_UNUSED(type);
    return BehaviorSilent;
}

bool TaskFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(flag);

    if (type == TypePermissions)
        return true;
    if (type == TypeRemoved) {
        deleteLater();
        return true;
    }
    return open(errorString, m_fileName);
}

void TaskFile::rename(const QString &newName)
{
    Q_UNUSED(newName);
}

bool TaskFile::open(QString *errorString, const QString &fileName)
{
    m_fileName = fileName;
    return TaskList::TaskListPlugin::instance()->loadFile(errorString, m_context, m_fileName);
}

ProjectExplorer::Project *TaskFile::context() const
{
    return m_context;
}

void TaskFile::setContext(ProjectExplorer::Project *context)
{
    m_context = context;
}

