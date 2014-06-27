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

#include "taskfilefactory.h"

#include "taskfile.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <coreplugin/icore.h>
#include <coreplugin/documentmanager.h>

#include <QMessageBox>

using namespace TaskList::Internal;

// --------------------------------------------------------------------------
// TaskFileFactory
// --------------------------------------------------------------------------

TaskFileFactory::TaskFileFactory(QObject * parent) :
    Core::IDocumentFactory(parent)
{
    setId("ProjectExplorer.TaskFileFactory");
    setDisplayName(tr("Task file reader"));
    addMimeType(QLatin1String("text/x-tasklist"));
    setOpener([this](const QString &fileName) -> Core::IDocument * {
        ProjectExplorer::Project *project = ProjectExplorer::ProjectExplorerPlugin::currentProject();
        return this->open(project ? project->projectDirectory().toString() : QString(), fileName);
    });
}

Core::IDocument *TaskFileFactory::open(const QString &base, const QString &fileName)
{
    foreach (TaskFile *doc, m_openFiles) {
        if (doc->filePath() == fileName)
            return doc;
    }

    TaskFile *file = new TaskFile(this);
    file->setBaseDir(base);

    QString errorString;
    if (!file->open(&errorString, fileName)) {
        QMessageBox::critical(Core::ICore::mainWindow(), tr("File Error"), errorString);
        delete file;
        return 0;
    }

    m_openFiles.append(file);

    // Register with filemanager:
    Core::DocumentManager::addDocument(file);

    return file;
}

void TaskFileFactory::closeAllFiles()
{
    foreach (TaskFile *document, m_openFiles)
        document->deleteLater();
    m_openFiles.clear();
}
