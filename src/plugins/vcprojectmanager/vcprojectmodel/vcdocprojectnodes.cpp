/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
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
#include "file.h"
#include "filecontainer.h"
#include "files.h"
#include "vcdocprojectnodes.h"
#include "vcprojectdocument.h"
#include "../vcprojectmanagerconstants.h"
#include "../widgets/filesettingswidget.h"

#include <projectexplorer/projectexplorer.h>

#include <QDir>
#include <QFileInfo>

namespace VcProjectManager {
namespace Internal {

VcFileNode::VcFileNode(IFile *fileModel, VcDocProjectNode *vcDocProject)
    : ProjectExplorer::FileNode(fileModel->canonicalPath(), fileModel->fileType(), false),
      m_vcFileModel(fileModel)
{
    Q_UNUSED(vcDocProject)
    connect(this, SIGNAL(settingsDialogAccepted()), vcDocProject, SIGNAL(settingsDialogAccepted()));
}

VcFileNode::~VcFileNode()
{
}

void VcFileNode::showSettingsWidget()
{
    FileSettingsWidget *settingsWidget = new FileSettingsWidget(m_vcFileModel);
    if (settingsWidget) {
        settingsWidget->show();
        connect(settingsWidget, SIGNAL(accepted()), this, SIGNAL(settingsDialogAccepted()));
    }
}

void VcFileNode::readChildren(VcDocProjectNode *vcDocProj)
{
    Q_UNUSED(vcDocProj)
}

VcFileContainerNode::VcFileContainerNode(IFileContainer *fileContainerModel, VcDocProjectNode *vcDocProjNode)
    : ProjectExplorer::FolderNode(fileContainerModel->displayName()),
      m_vcFileContainerModel(fileContainerModel),
      m_parentVcDocProjNode(vcDocProjNode)
{
    m_vcContainerType = VcContainerType_Filter;
    if (m_vcFileContainerModel->containerType() == QLatin1String(Constants::VC_PROJECT_FILE_CONTAINER_FOLDER))
        m_vcContainerType = VcContainerType_Folder;

    setProjectNode(vcDocProjNode);
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
    m_parentVcDocProjNode->addFileNodes(vcFileNodes);
    return true;
}

void VcFileContainerNode::addFileContainerNode(const QString &name, VcContainerType type)
{
    IFileContainer *fileContainerModel = 0;

    if (type == VcContainerType_Filter)
        fileContainerModel = new FileContainer(QLatin1String(Constants::VC_PROJECT_FILE_CONTAINER_FILTER),
                                               m_parentVcDocProjNode->m_vcProjectModel);
    else if (type == VcContainerType_Folder)
        fileContainerModel = new FileContainer(QLatin1String(Constants::VC_PROJECT_FILE_CONTAINER_FOLDER),
                                               m_parentVcDocProjNode->m_vcProjectModel);

    if (fileContainerModel) {
        fileContainerModel->setDisplayName(name);
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
                    vcFileContainerNode->m_vcFileContainerModel->displayName() == fileContainer->m_vcFileContainerModel->displayName())
                return false;
        }
    }

    m_vcFileContainerModel->addFileContainer(fileContainer->m_vcFileContainerModel);
    QList<ProjectExplorer::FolderNode *> vcFolderNodes;
    vcFolderNodes << fileContainer;
    m_parentVcDocProjNode->addFolderNodes(vcFolderNodes);
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
    m_parentVcDocProjNode->removeFolderNodes(folderNodesToRemove);

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
    m_parentVcDocProjNode->removeFileNodes(fileNodesToRemove);

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

    addFolderNodes(vcFolderNodes);

    QList<ProjectExplorer::FileNode *> vcFileNodes;

    for (int i = 0; i < m_vcFileContainerModel->fileCount(); ++i) {
        IFile *file = m_vcFileContainerModel->file(i);

        if (file)
            vcFileNodes.append(new VcFileNode(file, m_parentVcDocProjNode));
    }

    addFileNodes(vcFileNodes);
}

VcFileNode *VcFileContainerNode::findFileNode(const QString &filePath)
{
    VcFileNode *fileNode = 0;
    foreach (ProjectExplorer::FileNode *f, fileNodes()) {
        // There can be one match only here!
        if (f->path() != filePath)
            continue;
        fileNode = static_cast<VcFileNode *>(f);
        break;
    }

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

VcDocProjectNode::VcDocProjectNode(IVisualStudioProject *vcProjectModel)
    : ProjectExplorer::ProjectNode(vcProjectModel->filePath()),
      m_vcProjectModel(vcProjectModel)
{
    QList<ProjectExplorer::FolderNode *> vcFolderNodes;
    for (int i = 0; i < m_vcProjectModel->files()->fileContainerCount(); ++i) {
        IFileContainer *fileContainer = m_vcProjectModel->files()->fileContainer(i);
        if (fileContainer)
            vcFolderNodes.append(new VcFileContainerNode(fileContainer, this));
    }
    addFolderNodes(vcFolderNodes);

    QList<ProjectExplorer::FileNode *> vcFileNodes;
    for (int i = 0; i < m_vcProjectModel->files()->fileCount(); ++i) {
        IFile *file = m_vcProjectModel->files()->file(i);
        if (file)
            vcFileNodes.append(new VcFileNode(file, this));
    }
    addFileNodes(vcFileNodes);
}

VcDocProjectNode::~VcDocProjectNode()
{
}

bool VcDocProjectNode::hasBuildTargets() const
{
    return true;
}

QList<ProjectExplorer::ProjectAction> VcDocProjectNode::supportedActions(ProjectExplorer::Node *node) const
{
    QList<ProjectExplorer::ProjectAction> actions;

    actions << ProjectExplorer::AddNewFile
            << ProjectExplorer::AddExistingFile;

    ProjectExplorer::FileNode *fileNode = qobject_cast<ProjectExplorer::FileNode *>(node);
    if (fileNode && fileNode->fileType() != ProjectExplorer::ProjectFileType) {
        actions << ProjectExplorer::Rename;
        actions << ProjectExplorer::RemoveFile;
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

bool VcDocProjectNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    // add files like in VS
    //    if ()

    // add files to the node which called it
    ProjectExplorer::ProjectExplorerPlugin *projExplPlugin = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Node *node = projExplPlugin->currentNode();

    QStringList filesNotAdded;
    bool anyFileAdded = false;

    if (node->nodeType() == ProjectExplorer::FolderNodeType) {
        VcFileContainerNode *vcContainerNode = static_cast<VcFileContainerNode *>(node);

        if (vcContainerNode) {
            foreach (const QString &filePath, filePaths) {
                QString relativeFilePath = fileRelativePath(m_vcProjectModel->filePath(), filePath);

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

    if (node->nodeType() == ProjectExplorer::ProjectNodeType) {
        VcDocProjectNode *projectNode = static_cast<VcDocProjectNode *>(node);

        if (projectNode) {
            foreach (const QString &filePath, filePaths) {
                QString relativeFilePath = fileRelativePath(m_vcProjectModel->filePath(), filePath);

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

bool VcDocProjectNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    QStringList filesNotRemoved;

    foreach (const QString &filePath, filePaths) {
        QString relativeFilePath = fileRelativePath(m_vcProjectModel->filePath(), filePath);

        if (m_vcProjectModel->files()->fileExists(relativeFilePath)) {
            VcFileNode *fileNode = static_cast<VcFileNode *>(findFileNode(filePath));

            if (fileNode) {
                ProjectExplorer::FolderNode *parentNode = fileNode->parentFolderNode();

                if (parentNode && parentNode->nodeType() == ProjectExplorer::FolderNodeType) {
                    VcFileContainerNode *containerNode = static_cast<VcFileContainerNode *>(parentNode);

                    if (containerNode)
                        containerNode->removeFileNode(fileNode);
                }

                else if (parentNode && parentNode->nodeType() == ProjectExplorer::ProjectNodeType) {
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

bool VcDocProjectNode::deleteFiles(const QStringList &filePaths)
{
    Q_UNUSED(filePaths)
    return false;
}

bool VcDocProjectNode::renameFile(const QString &filePath, const QString &newFilePath)
{
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
        fileContainer = new FileContainer(QLatin1String(Constants::VC_PROJECT_FILE_CONTAINER_FILTER),
                                          m_vcProjectModel);
    else
        fileContainer = new FileContainer(QLatin1String(Constants::VC_PROJECT_FILE_CONTAINER_FOLDER),
                                          m_vcProjectModel);

    fileContainer->setDisplayName(name);
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
                    vcFileContainerNode->m_vcFileContainerModel->displayName() == fileContainerNode->m_vcFileContainerModel->displayName())
                return false;
        }
    }

    m_vcProjectModel->files()->addFileContainer(fileContainerNode->m_vcFileContainerModel);

    QList<ProjectExplorer::FolderNode *> vcFolderNodes;
    vcFolderNodes << fileContainerNode;
    addFolderNodes(vcFolderNodes);
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
    addFileNodes(vcFileNodes);
    return true;
}

void VcDocProjectNode::removeFileNode(VcFileNode *fileNode)
{
    if (!m_vcProjectModel || !fileNode || !fileNode->m_vcFileModel)
        return;

    QString relativePath = fileNode->m_vcFileModel->relativePath();

    QList<ProjectExplorer::FileNode *> fileNodesToRemove;
    fileNodesToRemove << fileNode;
    removeFileNodes(fileNodesToRemove);

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

    IFileContainer *fileContainer = fileContainerNode->m_vcFileContainerModel;

    QList<ProjectExplorer::FolderNode *> folderNodesToRemove;
    folderNodesToRemove << fileContainerNode;
    removeFolderNodes(folderNodesToRemove);

    m_vcProjectModel->files()->removeFileContainer(fileContainer);
    m_vcProjectModel->saveToFile(m_vcProjectModel->filePath());
}

void VcDocProjectNode::showSettingsDialog()
{
    if (m_vcProjectModel) {
        VcProjectDocumentWidget *settingsWidget = static_cast<VcProjectDocumentWidget *>(m_vcProjectModel->createSettingsWidget());

        if (settingsWidget) {
            settingsWidget->show();
            connect(settingsWidget, SIGNAL(accepted()), this, SIGNAL(settingsDialogAccepted()));
        }
    }
}

VcFileNode *VcDocProjectNode::findFileNode(const QString &filePath)
{
    VcFileNode *fileNode = 0;
    foreach (ProjectExplorer::FileNode *f, fileNodes()) {
        // There can be one match only here!
        if (f->path() != filePath)
            continue;
        fileNode = static_cast<VcFileNode *>(f);
        break;
    }

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

QString fileRelativePath(const QString &fullProjectPath, const QString &fullFilePath)
{
    QString relativePath = QFileInfo(fullProjectPath).absoluteDir().relativeFilePath(fullFilePath).replace(QLatin1String("/"), QLatin1String("\\"));

    if (!relativePath.startsWith(QLatin1String("..")))
        relativePath.prepend(QLatin1String(".\\"));

    return relativePath;
}

} // namespace Internal
} // namespace VcProjectManager
