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
#ifndef VCPROJECTMANAGER_INTERNAL_VCDOCPROJECTNODE_H
#define VCPROJECTMANAGER_INTERNAL_VCDOCPROJECTNODE_H

#include <projectexplorer/projectnodes.h>

namespace VcProjectManager {
namespace Internal {

class VcProjectDocument;
class IFileContainer;
class IFile;
class VcDocProjectNode;

class VcFileNode : public ProjectExplorer::FileNode
{
    Q_OBJECT

    friend class VcDocProjectNode;
    friend class VcFileContainerNode;

public:
    VcFileNode(IFile *fileModel, VcDocProjectNode *vcDocProject);
    ~VcFileNode();

protected:
    void readChildren(VcDocProjectNode *vcDocProj);

private:
    IFile *m_vcFileModel;
};

class VcFileContainerNode : public ProjectExplorer::FolderNode
{
    Q_OBJECT
    friend class VcFileNode;
    friend class VcDocProjectNode;

public:
    enum VcContainerType {
        VcContainerType_Filter,
        VcContainerType_Folder
    };

    VcFileContainerNode(IFileContainer *fileContainerModel, VcDocProjectNode *vcDocProjNode);
    ~VcFileContainerNode();

    VcContainerType containerType() const;

    void addFileNode(const QString &filePath);
    bool appendFileNode(VcFileNode *fileNode);

    void addFileContainerNode(const QString &name, VcContainerType type = VcContainerType_Filter);
    bool appendFileContainerNode(VcFileContainerNode *fileContainer);

    void removeFileContainerNode(VcFileContainerNode *filterNode);
    void removeFileNode(VcFileNode *fileNode);

    VcFileNode *findFileNode(const QString &filePath);
protected:
    void readChildren();

private:
    IFileContainer *m_vcFileContainerModel;
    VcContainerType m_vcContainerType;
    VcDocProjectNode *m_parentVcDocProjNode;
};

class VcDocProjectNode : public ProjectExplorer::ProjectNode
{
    Q_OBJECT

    friend class VcFileNode;
    friend class VcFileContainerNode;

public:
    VcDocProjectNode(VcProjectDocument* vcProjectModel);
    ~VcDocProjectNode();

    bool hasBuildTargets() const;
    QList<ProjectAction> supportedActions(Node *node) const;
    bool canAddSubProject(const QString &proFilePath) const;
    bool addSubProjects(const QStringList &proFilePaths);
    bool removeSubProjects(const QStringList &proFilePaths);
    bool addFiles(const ProjectExplorer::FileType fileType,
                  const QStringList &filePaths,
                  QStringList *notAdded);
    bool removeFiles(const ProjectExplorer::FileType fileType,
                     const QStringList &filePaths,
                     QStringList *notRemoved);
    bool deleteFiles(const ProjectExplorer::FileType fileType,
                     const QStringList &filePaths);
    bool renameFile(const ProjectExplorer::FileType fileType,
                     const QString &filePath,
                     const QString &newFilePath);
    QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node);

    QString projectDirectory() const;

    void addFileNode(const QString &filePath);
    void addFileContainerNode(const QString &name, VcFileContainerNode::VcContainerType type);
    bool appendFileContainerNode(VcFileContainerNode *fileContainerNode);
    bool appendFileNode(VcFileNode *fileNode);
    void removeFileNode(VcFileNode *fileNode);
    void removeFileContainerNode(VcFileContainerNode *fileContainerNode);

private:
    VcFileNode* findFileNode(const QString &filePath);

    VcProjectDocument *m_vcProjectModel;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCDOCPROJECTNODE_H
