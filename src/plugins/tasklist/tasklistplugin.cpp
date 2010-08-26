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

#include "tasklistplugin.h"

#include "taskfilefactory.h"
#include "tasklistmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QtPlugin>

using namespace TaskList::Internal;

TaskListPlugin::TaskListPlugin()
{ }

TaskListPlugin::~TaskListPlugin()
{ }

bool TaskListPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":tasklist/TaskList.mimetypes.xml"), errorMessage))
        return false;

    TaskListManager * manager = new TaskListManager(this);
    addAutoReleasedObject(new TaskFileFactory(manager));
    return true;
}

void TaskListPlugin::extensionsInitialized()
{ }

Q_EXPORT_PLUGIN(TaskListPlugin)
