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
#ifndef VCPROJECTMANAGER_INTERNAL_VCDOCPROJECTNODE_H
#define VCPROJECTMANAGER_INTERNAL_VCDOCPROJECTNODE_H

#include <projectexplorer/projectnodes.h>

namespace VcProjectManager {
namespace Internal {

class VcProjectDocument;
class Folder;
class Filter;
class File;
class VcDocProjectNode;

class VcFileNode : public ProjectExplorer::FileNode
{
public:
    VcFileNode(File *fileModel, VcDocProjectNode *vcDocProject);
    ~VcFileNode();

protected:
    void readChildren(VcDocProjectNode *vcDocProj);

private:
    File *m_vcFileModel;
};

class VcFolderNode : public ProjectExplorer::FolderNode
{
public:
    VcFolderNode(Folder *folderModel, VcDocProjectNode *vcDocProjNode);
    VcFolderNode(Filter *filterModel, VcDocProjectNode *vcDocProjNode);
    ~VcFolderNode();

    void appendFolderNode(VcFolderNode *folderNode);
    void appendFileNode(VcFileNode *fileNode);

protected:
    void readChildren(VcDocProjectNode *docProjNode);

private:
    Folder *m_vcFolderModel;
    Filter *m_vcFilterModel;
};

class VcDocProjectNode : public ProjectExplorer::ProjectNode
{
    friend class VcFolderNode;
    friend class VcFileNode;
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

private:
    VcProjectDocument *m_vcProjectModel;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCDOCPROJECTNODE_H
