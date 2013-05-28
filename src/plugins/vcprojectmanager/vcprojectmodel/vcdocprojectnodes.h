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
    Q_OBJECT

    friend class VcFolderNode;
    friend class VcDocProjectNode;
    friend class VcFilterNode;

public:
    VcFileNode(File *fileModel, VcDocProjectNode *vcDocProject);
    ~VcFileNode();

protected:
    void readChildren(VcDocProjectNode *vcDocProj);

private:
    File *m_vcFileModel;
};

class VcFilterNode;

class VcContainerNode : public ProjectExplorer::FolderNode
{
    Q_OBJECT

    friend class VcFileNode;
    friend class VcDocProjectNode;

public:
    enum VcContainerType {
        VcContainerType_Filter,
        VcContainerType_Folder
    };

    VcContainerNode(const QString &folderPath);
    ~VcContainerNode();

    VcContainerType vcContainerType() const;

    virtual void addFileNode(const QString &filePath) = 0;
    virtual bool appendFileNode(VcFileNode *fileNode) = 0;

    virtual void addFilterNode(const QString &name) = 0;
    virtual bool appendFilterNode(VcFilterNode *folderNode) = 0;

    virtual void removeFilterNode(VcFilterNode *filterNode) = 0;

protected:
    VcContainerType m_vcContainerType;
    VcDocProjectNode *m_parentVcDocProjNode;
};

class VcFilterNode : public VcContainerNode
{
    Q_OBJECT

    friend class VcFileNode;
    friend class VcDocProjectNode;
    friend class VcFolderNode;

public:
    VcFilterNode(Filter *filterModel, VcDocProjectNode *vcDocProjNode);
    ~VcFilterNode();

    void addFileNode(const QString &filePath);
    bool appendFileNode(VcFileNode *fileNode);

    void addFilterNode(const QString &name);
    bool appendFilterNode(VcFilterNode *folderNode);

    void removeFilterNode(VcFilterNode *filterNode);

protected:
    void readChildren();

private:
    Filter *m_vcFilterModel;
};

class VcFolderNode : public VcContainerNode
{
    Q_OBJECT

    friend class VcFileNode;
    friend class VcDocProjectNode;
    friend class VcFilterNode;

public:
    VcFolderNode(Folder *folderModel, VcDocProjectNode *vcDocProjNode);
    ~VcFolderNode();

    void addFileNode(const QString &filePath);
    bool appendFileNode(VcFileNode *fileNode);

    void addFilterNode(const QString &name);
    bool appendFilterNode(VcFilterNode *folderNode);

    void removeFilterNode(VcFilterNode *filterNode);

    void addFolderNode(const QString &name);    // only for VS 2005
    bool appendFolderNode(VcFolderNode *folderNode);    // only for VS 2005

    void removeFolderNode(VcFolderNode *node);

protected:
    void readChildren();

private:
    Folder *m_vcFolderModel;
};

class VcDocProjectNode : public ProjectExplorer::ProjectNode
{
    Q_OBJECT

    friend class VcFolderNode;
    friend class VcFileNode;
    friend class VcFilterNode;

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
    void addFolderNode(const QString &name);    // only for VS 2005
    void addFilterNode(const QString &name);
    bool appendFolderNode(VcFolderNode *folderNode);    // only for VS 2005
    bool appendFilterNode(VcFilterNode *filterNode);
    bool appendFileNode(VcFileNode *fileNode);

    void removeFilterNode(VcFilterNode *filterNode);
    void removeFolderNode(VcFolderNode *folderNode);

private:
    VcProjectDocument *m_vcProjectModel;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCDOCPROJECTNODE_H
