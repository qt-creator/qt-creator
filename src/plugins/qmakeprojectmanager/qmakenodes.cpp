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

#include "qmakenodes.h"
#include "qmakeproject.h"
#include "qmakerunconfigurationfactory.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <resourceeditor/resourcenode.h>

#include <utils/stringutils.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {

/*!
  \class QmakePriFileNode
  Implements abstract ProjectNode class
  */

QmakePriFileNode::QmakePriFileNode(QmakeProject *project, QmakeProFileNode *qmakeProFileNode,
                                   const FileName &filePath) :
    ProjectNode(filePath),
    m_project(project),
    m_qmakeProFileNode(qmakeProFileNode)
{ }

QmakePriFile *QmakePriFileNode::priFile() const
{
    return m_project->rootProFile()->findPriFile(filePath());
}

bool QmakePriFileNode::deploysFolder(const QString &folder) const
{
    QmakePriFile *pri = priFile();
    return pri ? pri->deploysFolder(folder) : false;
}

QList<RunConfiguration *> QmakePriFileNode::runConfigurations() const
{
    QmakeRunConfigurationFactory *factory = QmakeRunConfigurationFactory::find(m_project->activeTarget());
    if (factory)
        return factory->runConfigurationsForNode(m_project->activeTarget(), this);
    return QList<RunConfiguration *>();
}

QmakeProFileNode *QmakePriFileNode::proFileNode() const
{
    return m_qmakeProFileNode;
}

QList<ProjectAction> QmakePriFileNode::supportedActions(Node *node) const
{
    QList<ProjectAction> actions;

    const FolderNode *folderNode = this;
    const QmakeProFileNode *proFileNode;
    while (!(proFileNode = dynamic_cast<const QmakeProFileNode*>(folderNode)))
        folderNode = folderNode->parentFolderNode();
    Q_ASSERT(proFileNode);
    const QmakeProFile *pro = proFileNode->proFile();

    switch (pro ? pro->projectType() : ProjectType::Invalid) {
    case ProjectType::ApplicationTemplate:
    case ProjectType::StaticLibraryTemplate:
    case ProjectType::SharedLibraryTemplate:
    case ProjectType::AuxTemplate: {
        // TODO: Some of the file types don't make much sense for aux
        // projects (e.g. cpp). It'd be nice if the "add" action could
        // work on a subset of the file types according to project type.

        actions << AddNewFile;
        if (pro && pro->knowsFile(node->filePath()))
            actions << EraseFile;
        else
            actions << RemoveFile;

        bool addExistingFiles = true;
        if (node->nodeType() == NodeType::VirtualFolder) {
            // A virtual folder, we do what the projectexplorer does
            FolderNode *folder = node->asFolderNode();
            if (folder) {
                QStringList list;
                foreach (FolderNode *f, folder->folderNodes())
                    list << f->filePath().toString() + QLatin1Char('/');
                if (deploysFolder(Utils::commonPath(list)))
                    addExistingFiles = false;
            }
        }

        addExistingFiles = addExistingFiles && !deploysFolder(node->filePath().toString());

        if (addExistingFiles)
            actions << AddExistingFile << AddExistingDirectory;

        break;
    }
    case ProjectType::SubDirsTemplate:
        actions << AddSubProject << RemoveSubProject;
        break;
    default:
        break;
    }

    FileNode *fileNode = node->asFileNode();
    if ((fileNode && fileNode->fileType() != FileType::Project)
            || dynamic_cast<ResourceEditor::ResourceTopLevelNode *>(node)) {
        actions << Rename;
        actions << DuplicateFile;
    }

    Target *target = m_project->activeTarget();
    QmakeRunConfigurationFactory *factory = QmakeRunConfigurationFactory::find(target);
    if (factory && !factory->runConfigurationsForNode(target, node).isEmpty())
        actions << HasSubProjectRunConfigurations;

    return actions;
}

bool QmakePriFileNode::canAddSubProject(const QString &proFilePath) const
{
    QmakePriFile *pri = priFile();
    return pri ? pri->canAddSubProject(proFilePath) : false;
}

bool QmakePriFileNode::addSubProject(const QString &proFilePath)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->addSubProject(proFilePath) : false;
}

bool QmakePriFileNode::removeSubProject(const QString &proFilePath)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->removeSubProjects(proFilePath) : false;
}

bool QmakePriFileNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->addFiles(filePaths, notAdded) : false;
}

bool QmakePriFileNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->removeFiles(filePaths, notRemoved) : false;
}

bool QmakePriFileNode::deleteFiles(const QStringList &filePaths)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->deleteFiles(filePaths) : false;
}

bool QmakePriFileNode::canRenameFile(const QString &filePath, const QString &newFilePath)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->canRenameFile(filePath, newFilePath) : false;
}

bool QmakePriFileNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    QmakePriFile *pri = priFile();
    return pri ? pri->renameFile(filePath, newFilePath) : false;
}

FolderNode::AddNewInformation QmakePriFileNode::addNewInformation(const QStringList &files, Node *context) const
{
    Q_UNUSED(files)
    return FolderNode::AddNewInformation(filePath().fileName(), context && context->parentProjectNode() == this ? 120 : 90);
}

QmakeProFileNode *QmakeProFileNode::findProFileFor(const FileName &fileName) const
{
    if (fileName == filePath())
        return const_cast<QmakeProFileNode *>(this);
    for (Node *node : nodes()) {
        if (QmakeProFileNode *qmakeProFileNode = dynamic_cast<QmakeProFileNode *>(node))
            if (QmakeProFileNode *result = qmakeProFileNode->findProFileFor(fileName))
                return result;
    }
    return nullptr;
}

/*!
  \class QmakeProFileNode
  Implements abstract ProjectNode class
  */
QmakeProFileNode::QmakeProFileNode(QmakeProject *project, const FileName &filePath) :
    QmakePriFileNode(project, this, filePath)
{ }

bool QmakeProFileNode::showInSimpleTree() const
{
    return showInSimpleTree(projectType()) || m_project->rootProjectNode() == this;
}

QmakeProFile *QmakeProFileNode::proFile() const
{
    return m_project->rootProFile()->findProFile(filePath());
}

FolderNode::AddNewInformation QmakeProFileNode::addNewInformation(const QStringList &files, Node *context) const
{
    Q_UNUSED(files)
    return AddNewInformation(filePath().fileName(), context && context->parentProjectNode() == this ? 120 : 100);
}

bool QmakeProFileNode::showInSimpleTree(ProjectType projectType) const
{
    return projectType == ProjectType::ApplicationTemplate
            || projectType == ProjectType::SharedLibraryTemplate
            || projectType == ProjectType::StaticLibraryTemplate;
}

ProjectType QmakeProFileNode::projectType() const
{
    const QmakeProFile *pro = proFile();
    return pro ? pro->projectType() : ProjectType::Invalid;
}

QStringList QmakeProFileNode::variableValue(const Variable var) const
{
    QmakeProFile *pro = proFile();
    return pro ? pro->variableValue(var) : QStringList();
}

QString QmakeProFileNode::singleVariableValue(const Variable var) const
{
    const QStringList &values = variableValue(var);
    return values.isEmpty() ? QString() : values.first();
}

QString QmakeProFileNode::buildDir() const
{
    if (Target *target = m_project->activeTarget()) {
        if (BuildConfiguration *bc = target->activeBuildConfiguration()) {
            const QDir srcDirRoot(m_project->projectDirectory().toString());
            const QString relativeDir = srcDirRoot.relativeFilePath(filePath().parentDir().toString());
            return QDir::cleanPath(QDir(bc->buildDirectory().toString()).absoluteFilePath(relativeDir));
        }
    }
    return QString();
}

} // namespace QmakeProjectManager
