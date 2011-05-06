/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "taskfilefactory.h"

#include "taskfile.h"

#include <projectexplorer/projectexplorer.h>
#include <coreplugin/icore.h>
#include <coreplugin/filemanager.h>

#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>

using namespace TaskList::Internal;

// --------------------------------------------------------------------------
// TaskFileFactory
// --------------------------------------------------------------------------

TaskFileFactory::TaskFileFactory(QObject * parent) :
    Core::IFileFactory(parent),
    m_mimeTypes(QStringList() << QLatin1String("text/x-tasklist"))
{ }

TaskFileFactory::~TaskFileFactory()
{ }

QStringList TaskFileFactory::mimeTypes() const
{
    return m_mimeTypes;
}

QString TaskFileFactory::id() const
{
    return QLatin1String("ProjectExplorer.TaskFileFactory");
}

QString TaskFileFactory::displayName() const
{
    return tr("Task file reader");
}

Core::IFile *TaskFileFactory::open(const QString &fileName)
{
    ProjectExplorer::Project * context =
        ProjectExplorer::ProjectExplorerPlugin::instance()->currentProject();
    return open(context, fileName);
}

Core::IFile *TaskFileFactory::open(ProjectExplorer::Project *context, const QString &fileName)
{
    TaskFile *file = new TaskFile(this);
    file->setContext(context);

    QString errorString;
    if (!file->open(&errorString, fileName)) {
        QMessageBox::critical(Core::ICore::instance()->mainWindow(), tr("File Error"), errorString);
        delete file;
        return 0;
    }

    m_openFiles.append(file);

    // Register with filemanager:
    Core::ICore::instance()->fileManager()->addFile(file);

    return file;
}

void TaskFileFactory::closeAllFiles()
{
    foreach(Core::IFile *file, m_openFiles)
        file->deleteLater();
    m_openFiles.clear();
}
