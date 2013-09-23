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


VcContainerNode::VcContainerNode(const QString &folderPath)
    : ProjectExplorer::FolderNode(folderPath)
{
}

VcContainerNode::~VcContainerNode()
{
}

VcContainerNode::VcContainerType VcContainerNode::vcContainerType() const
{
    return m_vcContainerType;
}

VcFileNode *VcContainerNode::findFileNode(const QString &filePath)
{
    VcFileNode *fileNode = static_cast<VcFileNode *>(findFile(filePath));

    if (fileNode)
        return fileNode;

    foreach (ProjectExplorer::FolderNode *folderNode, m_subFolderNodes) {
        VcContainerNode *containerNode = static_cast<VcContainerNode *>(folderNode);
        fileNode = containerNode->findFileNode(filePath);

        if (fileNode)
            return fileNode;
    }

    return 0;
}


VcFilterNode::VcFilterNode(VcProjectManager::Internal::IFileContainer *filterModel, VcDocProjectNode *vcDocProjNode)
    : VcContainerNode(filterModel->name()),
      m_vcFilterModel(filterModel)
{
    m_parentVcDocProjNode = vcDocProjNode;
    m_vcContainerType = VcContainerType_Filter;
    readChildren();
}

VcFilterNode::~VcFilterNode()
{
}

void VcFilterNode::addFileNode(const QString &filePath)
{
    File *file = new File(m_parentVcDocProjNode->m_vcProjectModel);
    file->setRelativePath(filePath);
    VcFileNode *fileNode = new VcFileNode(file, m_parentVcDocProjNode);

    if (!appendFileNode(fileNode)) {
        delete file;
        delete fileNode;
    }
}

bool VcFilterNode::appendFileNode(VcFileNode *fileNode)
{
    if (!m_vcFilterModel || !fileNode || !fileNode->m_vcFileModel)
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

    m_vcFilterModel->addFile(fileNode->m_vcFileModel);
    QList<ProjectExplorer::FileNode *> vcFileNodes;
    vcFileNodes << fileNode;
    m_parentVcDocProjNode->addFileNodes(vcFileNodes, this);
    return true;
}

void VcFilterNode::addFilterNode(const QString &name)
{
    Filter *filter = new Filter(QLatin1String("Filter"), m_parentVcDocProjNode->m_vcProjectModel);
    filter->setName(name);
    VcFilterNode *filterNode = new VcFilterNode(filter, m_parentVcDocProjNode);

    if (!appendFilterNode(filterNode)) {
        delete filter;
        delete filterNode;
    }
}

bool VcFilterNode::appendFilterNode(VcFilterNode *filterNode)
{
    if (!m_vcFilterModel || !filterNode || !filterNode->m_vcFilterModel)
        return false;

    if (m_subFolderNodes.contains(filterNode))
        return false;
    else {
        foreach (ProjectExplorer::FolderNode *fNode, m_subFolderNodes) {
            VcFilterNode *vcFilterNode = qobject_cast<VcFilterNode *>(fNode);

            if (vcFilterNode &&
                    vcFilterNode->m_vcFilterModel &&
                    vcFilterNode->m_vcFilterModel->name() == filterNode->m_vcFilterModel->name())
                return false;
        }
    }

    m_vcFilterModel->addFileContainer(filterNode->m_vcFilterModel);
    QList<ProjectExplorer::FolderNode *> vcFolderNodes;
    vcFolderNodes << filterNode;
    m_parentVcDocProjNode->addFolderNodes(vcFolderNodes, this);
    m_parentVcDocProjNode->m_vcProjectModel->saveToFile(m_parentVcDocProjNode->m_vcProjectModel->filePath());
    return true;
}

void VcFilterNode::removeFilterNode(VcFilterNode *filterNode)
{
    if (!filterNode || !filterNode->m_vcFilterModel)
        return;

    IFileContainer *filter = filterNode->m_vcFilterModel;

    QList<ProjectExplorer::FolderNode *> folderNodesToRemove;
    folderNodesToRemove << filterNode;
    m_parentVcDocProjNode->removeFolderNodes(folderNodesToRemove, this);

    m_vcFilterModel->removeFileContainer(filter);
    m_parentVcDocProjNode->m_vcProjectModel->saveToFile(m_parentVcDocProjNode->m_vcProjectModel->filePath());
}

void VcFilterNode::removeFileNode(VcFileNode *fileNode)
{
    if (!fileNode || !fileNode->m_vcFileModel)
        return;

    IFile *file = fileNode->m_vcFileModel;

    QList<ProjectExplorer::FileNode *> fileNodesToRemove;
    fileNodesToRemove << fileNode;
    m_parentVcDocProjNode->removeFileNodes(fileNodesToRemove, this);

    m_vcFilterModel->removeFile(file);
    m_parentVcDocProjNode->m_vcProjectModel->saveToFile(m_parentVcDocProjNode->m_vcProjectModel->filePath());
}

void VcFilterNode::readChildren()
{
    QList<ProjectExplorer::FolderNode *> vcFolderNodes;

    for (int i = 0; i < m_vcFilterModel->childCount(); ++i) {
        IFileContainer *fileCont = m_vcFilterModel->fileContainer(i);
        if (fileCont)
            vcFolderNodes.append(new VcFilterNode(fileCont, m_parentVcDocProjNode));
    }

    m_parentVcDocProjNode->addFolderNodes(vcFolderNodes, this);

    QList<ProjectExplorer::FileNode *> vcFileNodes;

    for (int i = 0; i < m_vcFilterModel->fileCount(); ++i) {
        IFile *file = m_vcFilterModel->file(i);

        if (file)
            vcFileNodes.append(new VcFileNode(file, m_parentVcDocProjNode));
    }

    m_parentVcDocProjNode->addFileNodes(vcFileNodes, this);
}


VcFolderNode::VcFolderNode(VcProjectManager::Internal::IFileContainer *folderModel, VcDocProjectNode *vcDocProjNode)
    : VcContainerNode(folderModel->name()),
      m_vcFolderModel(folderModel)
{
    m_parentVcDocProjNode = vcDocProjNode;
    m_vcContainerType = VcContainerType_Folder;
    readChildren();
}

VcFolderNode::~VcFolderNode()
{
}

void VcFolderNode::addFileNode(const QString &filePath)
{
    File *file = new File(m_parentVcDocProjNode->m_vcProjectModel);
    file->setRelativePath(filePath);
    VcFileNode *fileNode = new VcFileNode(file, m_parentVcDocProjNode);

    if (!appendFileNode(fileNode)) {
        delete file;
        delete fileNode;
    }
}

bool VcFolderNode::appendFileNode(VcFileNode *fileNode)
{
    if (!m_vcFolderModel || !fileNode || !fileNode->m_vcFileModel)
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

    m_vcFolderModel->addFile(fileNode->m_vcFileModel);
    QList<ProjectExplorer::FileNode *> vcFileNodes;
    vcFileNodes << fileNode;
    m_parentVcDocProjNode->addFileNodes(vcFileNodes, this);
    return true;
}

void VcFolderNode::addFilterNode(const QString &name)
{
    Filter *filter = new Filter(QLatin1String("Filter"), m_parentVcDocProjNode->m_vcProjectModel);
    filter->setName(name);
    VcFilterNode *filterNode = new VcFilterNode(filter, m_parentVcDocProjNode);

    if (!appendFilterNode(filterNode)) {
        delete filter;
        delete filterNode;
    }
}

bool VcFolderNode::appendFilterNode(VcFilterNode *filterNode)
{
    if (!m_vcFolderModel && !filterNode && !filterNode->m_vcFilterModel)
        return false;

    if (m_subFolderNodes.contains(filterNode))
        return false;
    else {
        foreach (ProjectExplorer::FolderNode *fNode, m_subFolderNodes) {
            VcFilterNode *vcFilterNode = qobject_cast<VcFilterNode *>(fNode);

            if (vcFilterNode &&
                    vcFilterNode->m_vcFilterModel &&
                    vcFilterNode->m_vcFilterModel->name() == filterNode->m_vcFilterModel->name())
                return false;
        }
    }

    m_vcFolderModel->addFileContainer(filterNode->m_vcFilterModel);
    QList<ProjectExplorer::FolderNode *> vcFolderNodes;
    vcFolderNodes << filterNode;
    m_parentVcDocProjNode->addFolderNodes(vcFolderNodes, this);
    m_parentVcDocProjNode->m_vcProjectModel->saveToFile(m_parentVcDocProjNode->m_vcProjectModel->filePath());
    return true;
}

void VcFolderNode::removeFileNode(VcFileNode *fileNode)
{
    if (!fileNode || !fileNode->m_vcFileModel)
        return;

    IFile *file = fileNode->m_vcFileModel;

    QList<ProjectExplorer::FileNode *> fileNodesToRemove;
    fileNodesToRemove << fileNode;
    m_parentVcDocProjNode->removeFileNodes(fileNodesToRemove, this);

    m_vcFolderModel->removeFile(file);
    m_parentVcDocProjNode->m_vcProjectModel->saveToFile(m_parentVcDocProjNode->m_vcProjectModel->filePath());
}

void VcFolderNode::removeFilterNode(VcFilterNode *filterNode)
{
    if (!filterNode || !filterNode->m_vcFilterModel)
        return;

    IFileContainer *filter = filterNode->m_vcFilterModel;

    QList<ProjectExplorer::FolderNode *> folderNodesToRemove;
    folderNodesToRemove << filterNode;
    m_parentVcDocProjNode->removeFolderNodes(folderNodesToRemove, this);

    m_vcFolderModel->removeFileContainer(filter);
    m_parentVcDocProjNode->m_vcProjectModel->saveToFile(m_parentVcDocProjNode->m_vcProjectModel->filePath());
}

void VcFolderNode::addFolderNode(const QString &name)
{
    Folder *folder = new Folder(QLatin1String("Folder"), m_parentVcDocProjNode->m_vcProjectModel);
    folder->setName(name);
    VcFolderNode *folderNode = new VcFolderNode(folder, m_parentVcDocProjNode);

    if (!appendFolderNode(folderNode)) {
        delete folder;
        delete folderNode;
    }
}

bool VcFolderNode::appendFolderNode(VcFolderNode *folderNode)
{
    if (!m_vcFolderModel || !folderNode || !folderNode->m_vcFolderModel)
        return false;

    if (m_subFolderNodes.contains(folderNode))
        return false;
    else {
        foreach (ProjectExplorer::FolderNode *fNode, m_subFolderNodes) {
            VcFolderNode *vcFolderNode = qobject_cast<VcFolderNode *>(fNode);

            if (vcFolderNode &&
                    vcFolderNode->m_vcFolderModel &&
                    vcFolderNode->m_vcFolderModel->name() == folderNode->m_vcFolderModel->name())
                return false;
        }
    }

    m_vcFolderModel->addFileContainer(folderNode->m_vcFolderModel);
    QList<ProjectExplorer::FolderNode *> vcFolderNodes;
    vcFolderNodes << folderNode;
    m_parentVcDocProjNode->addFolderNodes(vcFolderNodes, this);
    m_parentVcDocProjNode->m_vcProjectModel->saveToFile(m_parentVcDocProjNode->m_vcProjectModel->filePath());
    return true;
}

void VcFolderNode::removeFolderNode(VcFolderNode *folderNode)
{
    if (!folderNode || !folderNode->m_vcFolderModel)
        return;

    IFileContainer *folder = folderNode->m_vcFolderModel;

    QList<ProjectExplorer::FolderNode *> folderNodesToRemove;
    folderNodesToRemove << folderNode;
    m_parentVcDocProjNode->removeFolderNodes(folderNodesToRemove, this);

    m_vcFolderModel->removeFileContainer(folder);
    m_parentVcDocProjNode->m_vcProjectModel->saveToFile(m_parentVcDocProjNode->m_vcProjectModel->filePath());
}

void VcFolderNode::readChildren()
{
    QList<ProjectExplorer::FolderNode *> vcFolderNodes;

    for (int i = 0; i < m_vcFolderModel->childCount(); ++i) {
        IFileContainer *fileCont = m_vcFolderModel->fileContainer(i);
        if (fileCont && fileCont->containerType() == QLatin1String("Filter"))
            vcFolderNodes.append(new VcFilterNode(fileCont, m_parentVcDocProjNode));
        else if (fileCont && fileCont->containerType() == QLatin1String("Folder"))
            vcFolderNodes.append(new VcFolderNode(fileCont, m_parentVcDocProjNode));
    }

    m_parentVcDocProjNode->addFolderNodes(vcFolderNodes, this);

    QList<ProjectExplorer::FileNode *> vcFileNodes;

    for (int i = 0; i < m_vcFolderModel->fileCount(); ++i) {
        IFile *file = m_vcFolderModel->file(i);
        if (file)
            vcFileNodes.append(new VcFileNode(file, m_parentVcDocProjNode));
    }

    m_parentVcDocProjNode->addFileNodes(vcFileNodes, this);
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
            vcFolderNodes.append(new VcFilterNode(filter, this));

        addFolderNodes(vcFolderNodes, this);

        QList<IFileContainer *> folders = files2005->folders();
        vcFolderNodes.clear();
        foreach (IFileContainer *folder, folders)
            vcFolderNodes.append(new VcFolderNode(folder, this));

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
            vcFolderNodes.append(new VcFilterNode(filter, this));

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
        VcContainerNode *vcContainerNode = static_cast<VcContainerNode *>(node);

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
                    VcContainerNode *containerNode = static_cast<VcContainerNode *>(parentNode);

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

void VcDocProjectNode::addFolderNode(const QString &name)
{
    Folder *folder = new Folder(QLatin1String("Folder"), m_vcProjectModel);
    folder->setName(name);
    VcFolderNode *folderNode = new VcFolderNode(folder, this);

    if (!appendFolderNode(folderNode)) {
        delete folder;
        delete folderNode;
    }
}

void VcDocProjectNode::addFilterNode(const QString &name)
{
    Filter *filter = new Filter(QLatin1String("Filter"), m_vcProjectModel);
    filter->setName(name);
    VcFilterNode *folderNode = new VcFilterNode(filter, this);

    if (!appendFilterNode(folderNode)) {
        delete filter;
        delete folderNode;
    }
}

bool VcDocProjectNode::appendFolderNode(VcFolderNode *folderNode)
{
    if (!m_vcProjectModel || m_vcProjectModel->documentVersion() != VcDocConstants::DV_MSVC_2005)
        return false;

    if (!folderNode || !folderNode->m_vcFolderModel)
        return false;

    if (m_subFolderNodes.contains(folderNode))
        return false;
    else {
        foreach (ProjectExplorer::FolderNode *fNode, m_subFolderNodes) {
            VcFolderNode *vcFolderNode = qobject_cast<VcFolderNode *>(fNode);

            if (vcFolderNode &&
                    vcFolderNode->m_vcFolderModel &&
                    folderNode->m_vcFolderModel &&
                    vcFolderNode->m_vcFolderModel->name() == folderNode->m_vcFolderModel->name())
                return false;
        }
    }

    m_vcProjectModel->files().staticCast<Files2005>()->addFolder(folderNode->m_vcFolderModel);
    QList<ProjectExplorer::FolderNode *> vcFolderNodes;
    vcFolderNodes << folderNode;
    addFolderNodes(vcFolderNodes, this);
    m_vcProjectModel->saveToFile(m_vcProjectModel->filePath());
    return true;
}

bool VcDocProjectNode::appendFilterNode(VcFilterNode *filterNode)
{
    if (!m_vcProjectModel || !filterNode || !filterNode->m_vcFilterModel)
        return false;

    if (m_subFolderNodes.contains(filterNode))
        return false;
    else {
        foreach (ProjectExplorer::FolderNode *fNode, m_subFolderNodes) {
            VcFilterNode *vcFolderNode = qobject_cast<VcFilterNode *>(fNode);

            if (vcFolderNode->m_vcFilterModel && vcFolderNode->m_vcFilterModel->name() == filterNode->m_vcFilterModel->name())
                return false;
        }
    }

    m_vcProjectModel->files()->addFilter(filterNode->m_vcFilterModel);
    QList<ProjectExplorer::FolderNode *> vcFolderNodes;
    vcFolderNodes << filterNode;
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

void VcDocProjectNode::removeFilterNode(VcFilterNode *filterNode)
{
    if (!m_vcProjectModel || !filterNode || !filterNode->m_vcFilterModel)
        return;

    QString filterName = filterNode->m_vcFilterModel->name();

    QList<ProjectExplorer::FolderNode *> folderNodesToRemove;
    folderNodesToRemove << filterNode;
    removeFolderNodes(folderNodesToRemove, filterNode->parentFolderNode());

    m_vcProjectModel->files()->removeFilter(filterName);
    m_vcProjectModel->saveToFile(m_vcProjectModel->filePath());
}

void VcDocProjectNode::removeFolderNode(VcFolderNode *folderNode)
{
    if (!m_vcProjectModel || m_vcProjectModel->documentVersion() != VcDocConstants::DV_MSVC_2005)
        return;

    if (!folderNode || !folderNode->m_vcFolderModel)
        return;

    QString folderName = folderNode->m_vcFolderModel->name();

    QList<ProjectExplorer::FolderNode *> folderNodesToRemove;
    folderNodesToRemove << folderNode;
    removeFolderNodes(folderNodesToRemove, this);

    m_vcProjectModel->files().staticCast<Files2005>()->removeFolder(folderName);
    m_vcProjectModel->saveToFile(m_vcProjectModel->filePath());
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

VcFileNode *VcDocProjectNode::findFileNode(const QString &filePath)
{
    VcFileNode *fileNode = static_cast<VcFileNode *>(findFile(filePath));

    if (fileNode)
        return fileNode;

    foreach (ProjectExplorer::FolderNode *folderNode, m_subFolderNodes) {
        VcContainerNode *containerNode = static_cast<VcContainerNode *>(folderNode);
        fileNode = containerNode->findFileNode(filePath);

        if (fileNode)
            return fileNode;
    }

    return 0;
}

} // namespace Internal
} // namespace VcProjectManager
