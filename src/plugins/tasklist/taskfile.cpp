/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "taskfile.h"

#include "tasklistplugin.h"

#include <utils/fileutils.h>

using namespace TaskList;
using namespace TaskList::Internal;

// --------------------------------------------------------------------------
// TaskFile
// --------------------------------------------------------------------------

TaskFile::TaskFile(QObject *parent) : Core::IDocument(parent)
{
    setId("TaskList.TaskFile");
}

bool TaskFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(fileName);
    Q_UNUSED(autoSave)
    return false;
}

QString TaskFile::defaultPath() const
{
    return QString();
}

QString TaskFile::suggestedFileName() const
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
    return load(errorString, filePath().toString());
}

bool TaskFile::load(QString *errorString, const QString &fileName)
{
    setFilePath(Utils::FileName::fromString(fileName));
    return TaskListPlugin::loadFile(errorString, m_baseDir, fileName);
}

QString TaskFile::baseDir() const
{
    return m_baseDir;
}

void TaskFile::setBaseDir(const QString &base)
{
    m_baseDir = base;
}
