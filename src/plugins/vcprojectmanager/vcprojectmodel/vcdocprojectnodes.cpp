/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#include "vcdocprojectnodes.h"

#include "vcprojectdocument.h"
#include "folder.h"
#include "filter.h"
#include "file.h"
#include "files.h"
#include "../vcprojectmanagerconstants.h"

#include <QFileInfo>
#include <projectexplorer/projectexplorer.h>

namespace VcProjectManager {
namespace Internal {

VcFileNode::VcFileNode(IFile *fileModel, VcDocProjectNode *vcDocProject)
    : ProjectExplorer::FileNode(fileModel->canonicalPath(), fileModel->fileType(), false),
      m_vcFileModel(fileModel)
{
    Q_UNUSED(vcDocProject)
}

VcFileNode::~VcFileNode()
{
}

void VcFileNode::readChildren(VcDocProjectNode *vcDocProj)
{
    Q_UNUSED(vcDocProj)
}

VcFileContainerNode::VcFileContainerNode(IFileContainer *fileContainerModel, VcDocProjectNode *vcDocProjNode)
    : ProjectExplorer::FolderNode(fileContainerModel->name()),
      m_vcFileContainerModel(fileContainerModel),
      m_parentVcDocProjNode(vcDocProjNode)
{
    m_vcContainerType = VcContainerType_Filter;
    if (m_vcFileContainerModel->containerType() == QLatin1String(Constants::VC_PROJECT_FILE_CONTAINER_FOLDER))
        m_vcContainerType = VcContainerType_Folder;

    readChildren();
}

VcFileContainerNode::~VcFileContainerNode()
{

}

VcFileContainerNode::VcContainerType VcFileContainerNode::containerType() const
{
    if (m_vcFileContainerModel->containerType() == QLatin1String(Constants::VC_PROJECT_FILE_CONTAINER_FILTER))
        return VcContainerType_Filter;
    return VcContainerType_Folder;
}

void VcFileContainerNode::addFileNode(const QString &filePath)
{
    File *file = new File(m_parentVcDocProjNode->m_vcProjectModel);
    file->setRelativePath(filePath);
    VcFileNode *fileNode = new VcFileNode(file, m_parentVcDocProjNode);

    if (!appendFileNode(fileNode)) {
        delete file;
        delete fileNode;
    }
}

bool VcFileContainerNode::appendFileNode(VcFileNode *fileNode)
{
    if (!m_vcFileContainerModel || !fileNode || !fileNode->m_vcFileModel)
        return false;

    if (m_fileNodes.contains(fileNode))
        return false;
    else {
        foreach (ProjectExplorer::FileNode *fNode, m_fileNodes) {
            VcFileNode *vcFileNode = qobject_cast<VcFileNode *>(fNode);

            if (vcFileNode &&
                    vcFileNode->m_vcFileModel &&
                    vcFileNode->m_vcFileModel->relativePath() == fileNode->m_vcFileModel->relativePath())
                return false;
        }
    }

    m_vcFileContainerModel->addFile(fileNode->m_vcFileModel);
    QList<ProjectExplorer::FileNode *> vcFileNodes;
    vcFileNodes << fileNode;
    m_parentVcDocProjNode->addFileNodes(vcFileNodes, this);
    return true;
}

void VcFileContainerNode::addFileContainerNode(const QString &name, VcContainerType type)
{
    IFileContainer *fileContainerModel = 0;

    if (type == VcContainerType_Filter)
        fileContainerModel = new Filter(m_parentVcDocProjNode->m_vcProjectModel);
    else if (type == VcContainerType_Folder)
        fileContainerModel = new Folder(m_parentVcDocProjNode->m_vcProjectModel);

    if (fileContainerModel) {
        fileContainerModel->setName(name);
        VcFileContainerNode *folderNode = new VcFileContainerNode(fileContainerModel, m_parentVcDocProjNode);

        if (!appendFileContainerNode(folderNode)) {
            delete fileContainerModel;
            delete folderNode;
        }
    }
}

bool VcFileContainerNode::appendFileContainerNode(VcFileContainerNode *fileContainer)
{
    if (!m_vcFileContainerModel || !fileContainer || !fileContainer->m_vcFileContainerModel)
        return false;

    if (m_subFolderNodes.contains(fileContainer))
        return false;
    else {
        foreach (ProjectExplorer::FolderNode *fNode, m_subFolderNodes) {
            VcFileContainerNode *vcFileContainerNode = qobject_cast<VcFileContainerNode *>(fNode);

            if (vcFileContainerNode &&
                    vcFileContainerNode->m_vcFileContainerModel &&
                    vcFileContainerNode->m_vcFileContainerModel->name() == fileContainer->m_vcFileContainerModel->name())
                return false;
        }
    }

    m_vcFileContainerModel->addFileContainer(fileContainer->m_vcFileContainerModel);
    QList<ProjectExplorer::FolderNode *> vcFolderNodes;
    vcFolderNodes << fileContainer;
    m_parentVcDocProjNode->addFolderNodes(vcFolderNodes, this);
    m_parentVcDocProjNode->m_vcProjectModel->saveToFile(m_parentVcDocProjNode->m_vcProjectModel->filePath());
    return true;
}

void VcFileContainerNode::removeFileContainerNode(VcFileContainerNode *fileContainer)
{
    if (!fileContainer || !fileContainer->m_vcFileContainerModel)
        return;

    IFileContainer *fileContainerModel = fileContainer->m_vcFileContainerModel;

    QList<ProjectExplorer::FolderNode *> folderNodesToRemove;
    folderNodesToRemove << fileContainer;
    m_parentVcDocProjNode->removeFolderNodes(folderNodesToRemove, this);

    m_vcFileContainerModel->removeFileContainer(fileContainerModel);
    m_parentVcDocProjNode->m_vcProjectModel->saveToFile(m_parentVcDocProjNode->m_vcProjectModel->filePath());
}

void VcFileContainerNode::removeFileNode(VcFileNode *fileNode)
{
    if (!fileNode || !fileNode->m_vcFileModel)
        return;

    IFile *file = fileNode->m_vcFileModel;

    QList<ProjectExplorer::FileNode *> fileNodesToRemove;
    fileNodesToRemove << fileNode;
    m_parentVcDocProjNode->removeFileNodes(fileNodesToRemove, this);

    m_vcFileContainerModel->removeFile(file);
    m_parentVcDocProjNode->m_vcProjectModel->saveToFile(m_parentVcDocProjNode->m_vcProjectModel->filePath());
}

void VcFileContainerNode::readChildren()
{
    QList<ProjectExplorer::FolderNode *> vcFolderNodes;

    for (int i = 0; i < m_vcFileContainerModel->childCount(); ++i) {
        IFileContainer *fileCont = m_vcFileContainerModel->fileContainer(i);
        if (fileCont)
            vcFolderNodes.append(new VcFileContainerNode(fileCont, m_parentVcDocProjNode));
    }

    m_parentVcDocProjNode->addFolderNodes(vcFolderNodes, this);

    QList<ProjectExplorer::FileNode *> vcFileNodes;

    for (int i = 0; i < m_vcFileContainerModel->fileCount(); ++i) {
        IFile *file = m_vcFileContainerModel->file(i);

        if (file)
            vcFileNodes.append(new VcFileNode(file, m_parentVcDocProjNode));
    }

    m_parentVcDocProjNode->addFileNodes(vcFileNodes, this);
}

VcFileNode *VcFileContainerNode::findFileNode(const QString &filePath)
{
    VcFileNode *fileNode = static_cast<VcFileNode *>(findFile(filePath));

    if (fileNode)
        return fileNode;

    foreach (ProjectExplorer::FolderNode *folderNode, m_subFolderNodes) {
        VcFileContainerNode *containerNode = static_cast<VcFileContainerNode *>(folderNode);
        fileNode = containerNode->findFileNode(filePath);

        if (fileNode)
            return fileNode;
    }

    return 0;
}

VcDocProjectNode::VcDocProjectNode(VcProjectDocument *vcProjectModel)
    : ProjectExplorer::ProjectNode(vcProjectModel->filePath()),
      m_vcProjectModel(vcProjectModel)
{
    if (m_vcProjectModel->documentVersion() == VcDocConstants::DV_MSVC_2005) {
        QSharedPointer<Files2005> files2005 = m_vcProjectModel->files().staticCast<Files2005>();
        QList<IFileContainer *> filters = files2005->fileContainers();

        QList<ProjectExplorer::FolderNode *> vcFolderNodes;
        foreach (IFileContainer *filter, filters)
            vcFolderNodes.append(new VcFileContainerNode(filter, this));

        addFolderNodes(vcFolderNodes, this);

        QList<IFileContainer *> folders = files2005->folders();
        vcFolderNodes.clear();
        foreach (IFileContainer *folder, folders)
            vcFolderNodes.append(new VcFileContainerNode(folder, this));

        addFolderNodes(vcFolderNodes, this);

        QList<IFile *> files = files2005->files();
        QList<ProjectExplorer::FileNode *> vcFileNodes;
        foreach (IFile *file, files)
            vcFileNodes.append(new VcFileNode(file, this));

        addFileNodes(vcFileNodes, this);
    }

    else {
        QList<IFileContainer *> filters = m_vcProjectModel->files()->fileContainers();

        QList<ProjectExplorer::FolderNode *> vcFolderNodes;
        foreach (IFileContainer *filter, filters)
            vcFolderNodes.append(new VcFileContainerNode(filter, this));

        addFolderNodes(vcFolderNodes, this);

        QList<IFile *> files = m_vcProjectModel->files()->files();
        QList<ProjectExplorer::FileNode *> vcFileNodes;
        foreach (IFile *file, files)
            vcFileNodes.append(new VcFileNode(file, this));

        addFileNodes(vcFileNodes, this);
    }
}

VcDocProjectNode::~VcDocProjectNode()
{
}

bool VcDocProjectNode::hasBuildTargets() const
{
    return true;
}

QList<ProjectExplorer::ProjectNode::ProjectAction> VcDocProjectNode::supportedActions(ProjectExplorer::Node *node) const
{
    QList<ProjectExplorer::ProjectNode::ProjectAction> actions;

    actions << AddNewFile
            << AddExistingFile;

    ProjectExplorer::FileNode *fileNode = qobject_cast<ProjectExplorer::FileNode *>(node);
    if (fileNode && fileNode->fileType() != ProjectExplorer::ProjectFileType) {
        actions << Rename;
        actions << RemoveFile;
    }

    return actions;
}

bool VcDocProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
}

bool VcDocProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool VcDocProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool VcDocProjectNode::addFiles(const ProjectExplorer::FileType fileType, const QStringList &filePaths, QStringList *notAdded)
{
    // add files like in VS
    //    if ()

    // add files to the node which called it
    Q_UNUSED(fileType)

    ProjectExplorer::ProjectExplorerPlugin *projExplPlugin = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Node *node = projExplPlugin->currentNode();

    QStringList filesNotAdded;
    bool anyFileAdded = false;

    if (node->nodeType() == ProjectExplorer::FolderNodeType) {
        VcFileContainerNode *vcContainerNode = static_cast<VcFileContainerNode *>(node);

        if (vcContainerNode) {
            foreach (const QString &filePath, filePaths) {
                QString relativeFilePath = m_vcProjectModel->fileRelativePath(filePath);

                // if file is already in the project don't add it
                if (m_vcProjectModel->files()->fileExists(relativeFilePath))
                    filesNotAdded << filePath;

                else {
                    vcContainerNode->addFileNode(relativeFilePath);
                    anyFileAdded = true;
                }
            }
        }
    }

    if (node->nodeType() == ProjectExplorer::ProjectFileType) {
        VcDocProjectNode *projectNode = static_cast<VcDocProjectNode *>(node);

        if (projectNode) {
            foreach (const QString &filePath, filePaths) {
                QString relativeFilePath = m_vcProjectModel->fileRelativePath(filePath);

                // if file is already in the project don't add it
                if (m_vcProjectModel->files()->fileExists(relativeFilePath))
                    filesNotAdded << filePath;

                else {
                    projectNode->addFileNode(relativeFilePath);
                    anyFileAdded = true;
                }
            }
        }
    }

    if (notAdded)
        *notAdded = filesNotAdded;

    if (anyFileAdded)
        m_vcProjectModel->saveToFile(m_vcProjectModel->filePath());

    return filesNotAdded.isEmpty();
}

bool VcDocProjectNode::removeFiles(const ProjectExplorer::FileType fileType, const QStringList &filePaths, QStringList *notRemoved)
{
    Q_UNUSED(fileType)

    QStringList filesNotRemoved;

    foreach (const QString &filePath, filePaths) {
        QString relativeFilePath = m_vcProjectModel->fileRelativePath(filePath);

        if (m_vcProjectModel->files()->fileExists(relativeFilePath)) {
            VcFileNode *fileNode = static_cast<VcFileNode *>(findFileNode(filePath));

            if (fileNode) {
                ProjectExplorer::FolderNode *parentNode = fileNode->parentFolderNode();

                if (parentNode && parentNode->nodeType() == ProjectExplorer::FolderNodeType) {
                    VcFileContainerNode *containerNode = static_cast<VcFileContainerNode *>(parentNode);

                    if (containerNode)
                        containerNode->removeFileNode(fileNode);
                }

                else if (parentNode && parentNode->nodeType() == ProjectExplorer::ProjectFileType) {
                    VcDocProjectNode *projectNode = static_cast<VcDocProjectNode *>(parentNode);

                    if (projectNode)
                        projectNode->removeFileNode(fileNode);
                }
            }
        }

        else
            filesNotRemoved << filePath;
    }

    if (notRemoved)
        *notRemoved = filesNotRemoved;

    return filesNotRemoved.isEmpty();
}

bool VcDocProjectNode::deleteFiles(const ProjectExplorer::FileType fileType, const QStringList &filePaths)
{
    Q_UNUSED(fileType)
    Q_UNUSED(filePaths)
    return false;
}

bool VcDocProjectNode::renameFile(const ProjectExplorer::FileType fileType, const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(fileType)
    Q_UNUSED(filePath)
    Q_UNUSED(newFilePath)
    return false;
}

QList<ProjectExplorer::RunConfiguration *> VcDocProjectNode::runConfigurationsFor(ProjectExplorer::Node *node)
{
    Q_UNUSED(node)
    return QList<ProjectExplorer::RunConfiguration *>();
}

/**
 * @brief VcDocProjectNode::projectDirectory
 * @return absoolute directory path to project file (.vcproj).
 */

QString VcDocProjectNode::projectDirectory() const
{
    QFileInfo fileInfo(path());
    return fileInfo.canonicalPath();
}

void VcDocProjectNode::addFileNode(const QString &filePath)
{
    File *file = new File(m_vcProjectModel);
    file->setRelativePath(filePath);
    VcFileNode *fileNode = new VcFileNode(file, this);

    if (!appendFileNode(fileNode)) {
        delete file;
        delete fileNode;
    }
}

void VcDocProjectNode::addFileContainerNode(const QString &name, VcFileContainerNode::VcContainerType type)
{
    IFileContainer *fileContainer = 0;

    if (type == VcFileContainerNode::VcContainerType_Filter)
        fileContainer = new Filter(m_vcProjectModel);
    else
        fileContainer = new Folder(m_vcProjectModel);

    fileContainer->setName(name);
    VcFileContainerNode *folderNode = new VcFileContainerNode(fileContainer, this);

    if (!appendFileContainerNode(folderNode)) {
        delete fileContainer;
        delete folderNode;
    }
}

bool VcDocProjectNode::appendFileContainerNode(VcFileContainerNode *fileContainerNode)
{
    if (!m_vcProjectModel || !fileContainerNode || !fileContainerNode->m_vcFileContainerModel)
        return false;

    if (m_subFolderNodes.contains(fileContainerNode))
        return false;
    else {
        foreach (ProjectExplorer::FolderNode *fNode, m_subFolderNodes) {
            VcFileContainerNode *vcFileContainerNode = qobject_cast<VcFileContainerNode *>(fNode);

            if (vcFileContainerNode &&
                    vcFileContainerNode->m_vcFileContainerModel &&
                    fileContainerNode->m_vcFileContainerModel &&
                    vcFileContainerNode->m_vcFileContainerModel->name() == fileContainerNode->m_vcFileContainerModel->name())
                return false;
        }
    }

    VcFileContainerNode::VcContainerType type = fileContainerNode->containerType();
    if (type == VcFileContainerNode::VcContainerType_Filter)
        m_vcProjectModel->files()->addFilter(fileContainerNode->m_vcFileContainerModel);
    else
        m_vcProjectModel->files().staticCast<Files2005>()->addFolder(fileContainerNode->m_vcFileContainerModel);

    QList<ProjectExplorer::FolderNode *> vcFolderNodes;
    vcFolderNodes << fileContainerNode;
    addFolderNodes(vcFolderNodes, this);
    m_vcProjectModel->saveToFile(m_vcProjectModel->filePath());
    return true;
}

bool VcDocProjectNode::appendFileNode(VcFileNode *fileNode)
{
    if (!m_vcProjectModel || !fileNode || !fileNode->m_vcFileModel)
        return false;

    if (m_fileNodes.contains(fileNode))
        return false;
    else {
        foreach (ProjectExplorer::FileNode *fNode, m_fileNodes) {
            VcFileNode *vcFileNode = qobject_cast<VcFileNode *>(fNode);

            if (vcFileNode->m_vcFileModel && vcFileNode->m_vcFileModel->relativePath() == fileNode->m_vcFileModel->relativePath())
                return false;
        }
    }

    m_vcProjectModel->files()->addFile(fileNode->m_vcFileModel);
    QList<ProjectExplorer::FileNode *> vcFileNodes;
    vcFileNodes << fileNode;
    addFileNodes(vcFileNodes, this);
    return true;
}

void VcDocProjectNode::removeFileNode(VcFileNode *fileNode)
{
    if (!m_vcProjectModel || !fileNode || !fileNode->m_vcFileModel)
        return;

    QString relativePath = fileNode->m_vcFileModel->relativePath();

    QList<ProjectExplorer::FileNode *> fileNodesToRemove;
    fileNodesToRemove << fileNode;
    removeFileNodes(fileNodesToRemove, this);

    IFile *filePtr = m_vcProjectModel->files()->file(relativePath);

    m_vcProjectModel->files()->removeFile(filePtr);
    m_vcProjectModel->saveToFile(m_vcProjectModel->filePath());
}

void VcDocProjectNode::removeFileContainerNode(VcFileContainerNode *fileContainerNode)
{
    if (!m_vcProjectModel || m_vcProjectModel->documentVersion() != VcDocConstants::DV_MSVC_2005)
        return;

    if (!fileContainerNode || !fileContainerNode->m_vcFileContainerModel)
        return;

    QString containerName = fileContainerNode->m_vcFileContainerModel->name();
    VcFileContainerNode::VcContainerType type = fileContainerNode->containerType();

    QList<ProjectExplorer::FolderNode *> folderNodesToRemove;
    folderNodesToRemove << fileContainerNode;
    removeFolderNodes(folderNodesToRemove, this);

    if (type == VcFileContainerNode::VcContainerType_Folder)
        m_vcProjectModel->files().staticCast<Files2005>()->removeFolder(containerName);
    else if (type == VcFileContainerNode::VcContainerType_Filter)
        m_vcProjectModel->files()->removeFilter(containerName);

    m_vcProjectModel->saveToFile(m_vcProjectModel->filePath());
}

VcFileNode *VcDocProjectNode::findFileNode(const QString &filePath)
{
    VcFileNode *fileNode = static_cast<VcFileNode *>(findFile(filePath));

    if (fileNode)
        return fileNode;

    foreach (ProjectExplorer::FolderNode *folderNode, m_subFolderNodes) {
        VcFileContainerNode *containerNode = static_cast<VcFileContainerNode *>(folderNode);
        fileNode = containerNode->findFileNode(filePath);

        if (fileNode)
            return fileNode;
    }

    return 0;
}

} // namespace Internal
} // namespace VcProjectManager
