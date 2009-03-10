/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "genericprojectnodes.h"
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QtDebug>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

GenericProjectNode::GenericProjectNode(Core::IFile *projectFile)
    : ProjectExplorer::ProjectNode(QFileInfo(projectFile->fileName()).absolutePath()),
      _projectFile(projectFile)
{}

GenericProjectNode::~GenericProjectNode()
{ }

Core::IFile *GenericProjectNode::projectFile() const
{ return _projectFile; }

QString GenericProjectNode::projectFilePath() const
{ return _projectFile->fileName(); }

void GenericProjectNode::refresh()
{
    QSettings projectInfo(projectFilePath(), QSettings::IniFormat);

    _files        = projectInfo.value(QLatin1String("files")).toStringList();
    _generated    = projectInfo.value(QLatin1String("generated")).toStringList();
    _includePaths = projectInfo.value(QLatin1String("includes")).toStringList();
    _defines      = projectInfo.value(QLatin1String("defines")).toStringList();
    _toolChainId  = projectInfo.value(QLatin1String("toolchain"), QLatin1String("gcc")).toString().toLower();

    using namespace ProjectExplorer;

    QList<FileNode *> fileNodes;

    FileNode *projectFileNode = new FileNode(projectFilePath(), ProjectFileType, /*generated = */ false);
    fileNodes.append(projectFileNode);

    QDir projectPath(path());

    foreach (const QString &file, _files) {
        QFileInfo fileInfo(projectPath, file);
        QString filePath = fileInfo.absoluteFilePath();
        FileType fileType = SourceType; // ### FIXME

        FileNode *fileNode = new FileNode(filePath, fileType, /*generated = */ false);

        fileNodes.append(fileNode);
    }

    addFileNodes(fileNodes, this);
}

QStringList GenericProjectNode::files() const
{ return _files; }

QStringList GenericProjectNode::generated() const
{ return _generated; }

QStringList GenericProjectNode::includePaths() const
{ return _includePaths; }

QStringList GenericProjectNode::defines() const
{ return _defines; }

QString GenericProjectNode::toolChainId() const
{ return _toolChainId; }

bool GenericProjectNode::hasTargets() const
{
    qDebug() << Q_FUNC_INFO;

    return true;
}

QList<ProjectExplorer::ProjectNode::ProjectAction> GenericProjectNode::supportedActions() const
{
    qDebug() << Q_FUNC_INFO;

    return QList<ProjectAction>();
}

bool GenericProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    qDebug() << Q_FUNC_INFO;

    Q_UNUSED(proFilePaths);
    return false;
}

bool GenericProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    qDebug() << Q_FUNC_INFO;

    Q_UNUSED(proFilePaths);
    return false;
}

bool GenericProjectNode::addFiles(const ProjectExplorer::FileType fileType,
                                  const QStringList &filePaths, QStringList *notAdded)
{
    qDebug() << Q_FUNC_INFO;

    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    Q_UNUSED(notAdded);
    return false;
}

bool GenericProjectNode::removeFiles(const ProjectExplorer::FileType fileType,
                                     const QStringList &filePaths, QStringList *notRemoved)
{
    qDebug() << Q_FUNC_INFO;

    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    Q_UNUSED(notRemoved);
    return false;
}

bool GenericProjectNode::renameFile(const ProjectExplorer::FileType fileType,
                                    const QString &filePath, const QString &newFilePath)
{
    qDebug() << Q_FUNC_INFO;

    Q_UNUSED(fileType);
    Q_UNUSED(filePath);
    Q_UNUSED(newFilePath);
    return false;
}
