/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "project/nimproject.h"

#include "nimconstants.h"
#include "project/nimbuildconfiguration.h"
#include "project/nimprojectnode.h"
#include "project/nimprojectmanager.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <texteditor/textdocument.h>

#include <QDebug>
#include <QFileInfo>
#include <QQueue>
#include <QThread>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

const int MIN_TIME_BETWEEN_PROJECT_SCANS = 4500;

NimProject::NimProject(NimProjectManager *projectManager, const QString &fileName)
    : m_projectManager(projectManager)
    , m_document(new TextEditor::TextDocument)
{
    setDocument(m_document);
    setId(Constants::C_NIMPROJECT_ID);
    m_document->setFilePath(FileName::fromString(fileName));
    m_projectFile = QFileInfo(fileName);
    m_projectDir = m_projectFile.dir();
    m_rootNode = new NimProjectNode(FileName::fromString(m_projectDir.absolutePath()));
    m_rootNode->setDisplayName(m_projectDir.dirName());

    m_projectScanTimer.setSingleShot(true);
    connect(&m_projectScanTimer, &QTimer::timeout, this, &NimProject::populateProject);

    populateProject();

    connect(&m_fsWatcher, &QFileSystemWatcher::directoryChanged, this, &NimProject::scheduleProjectScan);
}

QString NimProject::displayName() const
{
    return m_projectDir.dirName();
}

IProjectManager *NimProject::projectManager() const
{
    return m_projectManager;
}

ProjectNode *NimProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList NimProject::files(FilesMode) const
{
    return QStringList(m_files.toList());
}

bool NimProject::needsConfiguration() const
{
    return targets().empty();
}

QString NimProject::projectDirectoryPath() const
{
    return m_projectDir.absolutePath();
}

QString NimProject::projectFilePath() const
{
    return m_projectFile.absoluteFilePath();
}

void NimProject::scheduleProjectScan()
{
    auto elapsedTime = m_lastProjectScan.elapsed();
    if (elapsedTime < MIN_TIME_BETWEEN_PROJECT_SCANS) {
        if (!m_projectScanTimer.isActive()) {
            m_projectScanTimer.setInterval(MIN_TIME_BETWEEN_PROJECT_SCANS - elapsedTime);
            m_projectScanTimer.start();
        }
    } else {
        populateProject();
    }
}

void NimProject::populateProject()
{
    m_lastProjectScan.start();

    QSet<QString> oldFiles(m_files);
    m_files.clear();

    recursiveScanDirectory(m_projectDir, m_files);

    QSet<QString> removedFiles = oldFiles - m_files;
    QSet<QString> addedFiles = m_files - oldFiles;

    removeNodes(removedFiles);
    addNodes(addedFiles);

    if (removedFiles.size() || addedFiles.size())
        emit fileListChanged();
}

void NimProject::recursiveScanDirectory(const QDir &dir, QSet<QString> &container)
{
    static const QRegExp projectFilePattern(QLatin1String(".*\\.nimproject(?:\\.user)?$"));
    for (const QFileInfo &info : dir.entryInfoList(QDir::AllDirs |
                                                   QDir::Files |
                                                   QDir::NoDotAndDotDot |
                                                   QDir::NoSymLinks |
                                                   QDir::CaseSensitive)) {
        if (info.isDir())
            recursiveScanDirectory(QDir(info.filePath()), container);
        else if (projectFilePattern.indexIn(info.fileName()) == -1)
            container << info.filePath();
    }
    m_fsWatcher.addPath(dir.absolutePath());
}

void NimProject::addNodes(const QSet<QString> &nodes)
{
    QStringList path;
    foreach (const QString &node, nodes) {
        path = m_projectDir.relativeFilePath(node).split(QDir::separator());
        path.pop_back();
        FolderNode *folder = findFolderFor(path);
        auto fileNode = new FileNode(FileName::fromString(node), SourceType, false);
        folder->addFileNodes({fileNode});
    }
}

void NimProject::removeNodes(const QSet<QString> &nodes)
{
    QStringList path;
    foreach (const QString &node, nodes) {
        path = m_projectDir.relativeFilePath(node).split(QDir::separator());
        path.pop_back();
        FolderNode *folder = findFolderFor(path);

        for (FileNode *fileNode : folder->fileNodes()) {
            if (fileNode->filePath().toString() == node) {
                folder->removeFileNodes({fileNode});
                break;
            }
        }
    }
}

FolderNode *NimProject::findFolderFor(const QStringList &path)
{
    FolderNode *folder = m_rootNode;

    QString currentPath = m_projectDir.absolutePath();

    foreach (const QString &part, path) {
        // Create relative path
        if (!currentPath.isEmpty())
            currentPath += QDir::separator();
        currentPath += part;

        // Find the child with the given name
        QList<FolderNode *> subFolderNodes = folder->subFolderNodes();
        auto predicate = [&part] (const FolderNode * f) {
            return f->displayName() == part;
        };
        auto it = std::find_if(subFolderNodes.begin(), subFolderNodes.end(), predicate);

        if (it != subFolderNodes.end()) {
            folder = *it;
            continue;
        }

        // Folder not found. Add it
        QString newFolderPath = QDir::cleanPath(currentPath + QDir::separator() + part);
        auto newFolder = new FolderNode(FileName::fromString(newFolderPath),
                                        FolderNodeType,
                                        part);
        folder->addFolderNodes({newFolder});
        folder = newFolder;
    }
    return folder;
}

bool NimProject::supportsKit(Kit *k, QString *) const
{
    return k->isValid();
}

FileNameList NimProject::nimFiles() const
{
    FileNameList result;

    QQueue<FolderNode *> folders;
    folders.enqueue(m_rootNode);

    while (!folders.isEmpty()) {
        FolderNode *folder = folders.takeFirst();
        for (FileNode *file : folder->fileNodes()) {
            if (file->displayName().endsWith(QLatin1String(".nim")))
                result.append(file->filePath());
        }
        folders.append(folder->subFolderNodes());
    }

    return result;
}

}
