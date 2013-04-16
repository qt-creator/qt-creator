/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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

namespace VcProjectManager {
namespace Internal {

VcFileNode::VcFileNode(File *fileModel, VcDocProjectNode *vcDocProject)
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


VcFolderNode::VcFolderNode(Folder *folderModel, VcDocProjectNode *vcDocProjNode)
    : FolderNode(folderModel->name()),
      m_vcFolderModel(folderModel),
      m_vcFilterModel(0)
{
    readChildren(vcDocProjNode);
}

VcFolderNode::VcFolderNode(Filter *filterModel, VcDocProjectNode *vcDocProjNode)
    : FolderNode(filterModel->name()),
      m_vcFilterModel(filterModel),
      m_vcFolderModel(0)
{
    readChildren(vcDocProjNode);
}

VcFolderNode::~VcFolderNode()
{
}

void VcFolderNode::appendFolderNode(VcFolderNode *folderNode)
{
    if (m_subFolderNodes.contains(folderNode))
        return;
    m_subFolderNodes.append(folderNode);
}

void VcFolderNode::appendFileNode(VcFileNode *fileNode)
{
    if (m_fileNodes.contains(fileNode))
        return;
    m_fileNodes.append(fileNode);
}

void VcFolderNode::readChildren(VcDocProjectNode *docProjNode)
{
    if (m_vcFilterModel) {
        QList<Filter::Ptr > filters = m_vcFilterModel->filters();
        QList<File::Ptr > files = m_vcFilterModel->files();

        QList<ProjectExplorer::FolderNode *> vcFolderNodes;
        foreach (Filter::Ptr filter, filters)
            vcFolderNodes.append(new VcFolderNode(filter.data(), docProjNode));

        docProjNode->addFolderNodes(vcFolderNodes, this);

        QList<ProjectExplorer::FileNode *> vcFileNodes;
        foreach (File::Ptr file, files)
            vcFileNodes.append(new VcFileNode(file.data(), docProjNode));

        docProjNode->addFileNodes(vcFileNodes, this);
    }

    else if (m_vcFolderModel) {
        QList<Filter::Ptr > filters = m_vcFolderModel->filters();
        QList<Folder::Ptr > folders = m_vcFolderModel->folders();
        QList<File::Ptr > files = m_vcFolderModel->files();

        QList<ProjectExplorer::FolderNode *> vcFolderNodes;
        foreach (Filter::Ptr filter, filters)
            vcFolderNodes.append(new VcFolderNode(filter.data(), docProjNode));

        docProjNode->addFolderNodes(vcFolderNodes, this);

        vcFolderNodes.clear();
        foreach (Folder::Ptr folder, folders)
            vcFolderNodes.append(new VcFolderNode(folder.data(), docProjNode));

        docProjNode->addFolderNodes(vcFolderNodes, this);

        QList<ProjectExplorer::FileNode *> vcFileNodes;
        foreach (File::Ptr file, files)
            vcFileNodes.append(new VcFileNode(file.data(), docProjNode));

        docProjNode->addFileNodes(vcFileNodes, this);
    }
}


VcDocProjectNode::VcDocProjectNode(VcProjectDocument *vcProjectModel)
    : ProjectExplorer::ProjectNode(vcProjectModel->filePath()),
      m_vcProjectModel(vcProjectModel)
{
    if (m_vcProjectModel->documentVersion() == VcDocConstants::DV_MSVC_2005) {
        QSharedPointer<Files2005> files2005 = m_vcProjectModel->files().staticCast<Files2005>();
        QList<Filter::Ptr > filters = files2005->filters();

        QList<ProjectExplorer::FolderNode *> vcFolderNodes;
        foreach (Filter::Ptr filter, filters)
            vcFolderNodes.append(new VcFolderNode(filter.data(), this));

        addFolderNodes(vcFolderNodes, this);

        QList<Folder::Ptr > folders = files2005->folders();
        vcFolderNodes.clear();
        foreach (Folder::Ptr folder, folders)
            vcFolderNodes.append(new VcFolderNode(folder.data(), this));

        addFolderNodes(vcFolderNodes, this);

        QList<File::Ptr > files = files2005->files();
        QList<ProjectExplorer::FileNode *> vcFileNodes;
        foreach (File::Ptr file, files)
            vcFileNodes.append(new VcFileNode(file.data(), this));

        addFileNodes(vcFileNodes, this);
    }

    else {
        QList<Filter::Ptr > filters = m_vcProjectModel->files()->filters();

        QList<ProjectExplorer::FolderNode *> vcFolderNodes;
        foreach (Filter::Ptr filter, filters)
            vcFolderNodes.append(new VcFolderNode(filter.data(), this));

        addFolderNodes(vcFolderNodes, this);

        QList<File::Ptr > files = m_vcProjectModel->files()->files();
        QList<ProjectExplorer::FileNode *> vcFileNodes;
        foreach (File::Ptr file, files)
            vcFileNodes.append(new VcFileNode(file.data(), this));

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
    Q_UNUSED(node)
    return QList<ProjectExplorer::ProjectNode::ProjectAction>();
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
    Q_UNUSED(fileType)
    Q_UNUSED(filePaths)
    Q_UNUSED(notAdded)
    return false;
}

bool VcDocProjectNode::removeFiles(const ProjectExplorer::FileType fileType, const QStringList &filePaths, QStringList *notRemoved)
{
    Q_UNUSED(fileType)
    Q_UNUSED(filePaths)
    Q_UNUSED(notRemoved)
    return false;
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

} // namespace Internal
} // namespace VcProjectManager
