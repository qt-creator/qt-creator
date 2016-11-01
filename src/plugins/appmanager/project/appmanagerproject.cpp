/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include "appmanagerproject.h"
#include "appmanagerprojectnode.h"
#include "appmanagerprojectmanager.h"

#include "../appmanagerconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <texteditor/textdocument.h>
#include <projectexplorer/kitinformation.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtsupportconstants.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/deploymentdata.h>

#include <QFileInfo>
#include <QQueue>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager {

const qint64 MIN_TIME_BETWEEN_PROJECT_SCANS = 4500;

AppManagerProject::AppManagerProject(AppManagerProjectManager *projectManager, const QString &fileName)
    : m_projectManager(projectManager)
    , m_document(new TextEditor::TextDocument)
{
    setDocument(m_document);
    setId(Constants::C_APPMANAGERPROJECT_ID);
    m_document->setFilePath(FileName::fromString(fileName));
    m_rootNode = new AppManagerProjectNode(projectDirectory());
    m_rootNode->setDisplayName(displayName());

    m_projectScanTimer.setSingleShot(true);
    connect(&m_projectScanTimer, &QTimer::timeout, this, &AppManagerProject::populateProject);

    populateProject();

    connect(&m_fsWatcher, &QFileSystemWatcher::directoryChanged, this, &AppManagerProject::scheduleProjectScan);
}

QString AppManagerProject::displayName() const
{
    return projectDirectory().toUserOutput();
}

IProjectManager *AppManagerProject::projectManager() const
{
    return m_projectManager;
}

ProjectNode *AppManagerProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList AppManagerProject::files(FilesMode) const
{
    return QStringList(m_files.toList());
}

bool AppManagerProject::needsConfiguration() const
{
    return false;
}

void AppManagerProject::scheduleProjectScan()
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

void AppManagerProject::populateProject()
{
    m_lastProjectScan.start();

    QSet<QString> oldFiles(m_files);
    m_files.clear();

    recursiveScanDirectory(projectDirectory().toString(), m_files);

    QSet<QString> removedFiles = oldFiles - m_files;
    QSet<QString> addedFiles = m_files - oldFiles;

    removeNodes(removedFiles);
    addNodes(addedFiles);

    if (removedFiles.size() || addedFiles.size()) {
        emit fileListChanged();

        auto files = deployableFiles();
        foreach (ProjectExplorer::Target *target, targets())
            targetUpdateDeployableFiles(target, files);
    }

    emit parsingFinished();
}

void AppManagerProject::recursiveScanDirectory(const QDir &dir, QSet<QString> &container)
{
    for (const QFileInfo &info : dir.entryInfoList(QDir::AllDirs |
                                                   QDir::Files |
                                                   QDir::NoDotAndDotDot |
                                                   QDir::NoSymLinks |
                                                   QDir::CaseSensitive)) {
        if (info.isDir())
            recursiveScanDirectory(QDir(info.filePath()), container);
        else
            container << info.filePath();
    }

    m_fsWatcher.addPath(dir.absolutePath());
}

void AppManagerProject::addNodes(const QSet<QString> &nodes)
{
    QStringList path;
    foreach (const QString &node, nodes) {
        path = QDir(projectDirectory().toString()).relativeFilePath(node).split(QDir::separator());
        path.pop_back();
        FolderNode *folder = findFolderFor(path);
        auto fileNode = new FileNode(FileName::fromString(node), SourceType, false);
        folder->addFileNodes({fileNode});
    }
}

void AppManagerProject::removeNodes(const QSet<QString> &nodes)
{
    QStringList path;
    foreach (const QString &node, nodes) {
        path = QDir(projectDirectory().toString()).relativeFilePath(node).split(QDir::separator());
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

FolderNode *AppManagerProject::findFolderFor(const QStringList &path)
{
    FolderNode *folder = m_rootNode;

    QString currentPath = projectDirectory().toString();

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

bool AppManagerProject::supportsKit(Kit *k, QString *) const
{
    return k->isValid();
}

Project::RestoreResult AppManagerProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    auto result = Project::fromMap(map, errorMessage);

    if (result != RestoreResult::Ok)
        return result;

    populateProject();

    if (!activeTarget()) {
        auto kits = KitManager::matchingKits(KitMatcher([] (const Kit *kit) -> bool {
            return DeviceKitInformation::device(kit) != nullptr;
        }));

        for (auto kit : kits)
            addTarget(createTarget(kit));
    }

    foreach (Target *t, targets()) {
        addedTarget(t);
        targetUpdateDeployableFiles(t, deployableFiles());
    }

    return RestoreResult::Ok;
}

QList<ProjectExplorer::DeployableFile> AppManagerProject::deployableFiles() const
{
    QList<ProjectExplorer::DeployableFile> files;

    foreach (const QString &file, m_files)
        if (!file.endsWith(".yaml.user"))
        files << ProjectExplorer::DeployableFile(file, QStringLiteral("/tmp/deploy-") + displayName());

    return files;
}

void AppManagerProject::targetUpdateDeployableFiles(ProjectExplorer::Target *target, const QList<ProjectExplorer::DeployableFile> &files)
{
    auto data = target->deploymentData();
    data.setFileList(files);
    target->setDeploymentData(data);
}

} // namespace AppManager
