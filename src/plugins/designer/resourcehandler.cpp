/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "resourcehandler.h"
#include "designerconstants.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/nodesvisitor.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <resourceeditor/resourcenode.h>

#include <QDesignerFormWindowInterface>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace Designer {
namespace Internal {

// Visit project nodes and collect qrc-files.
class QrcFilesVisitor : public NodesVisitor
{
public:
    QStringList qrcFiles() const;

    void visitProjectNode(ProjectNode *node);
    void visitFolderNode(FolderNode *node);
private:
    QStringList m_qrcFiles;
};

QStringList QrcFilesVisitor::qrcFiles() const
{
    return m_qrcFiles;
}

void QrcFilesVisitor::visitProjectNode(ProjectNode *projectNode)
{
    visitFolderNode(projectNode);
}

void QrcFilesVisitor::visitFolderNode(FolderNode *folderNode)
{
    foreach (const FileNode *fileNode, folderNode->fileNodes()) {
        if (fileNode->fileType() == ResourceType)
            m_qrcFiles.append(fileNode->path().toString());
    }
    if (dynamic_cast<ResourceEditor::ResourceTopLevelNode *>(folderNode))
        m_qrcFiles.append(folderNode->path().toString());
}

// ------------ ResourceHandler
ResourceHandler::ResourceHandler(QDesignerFormWindowInterface *fw) :
    QObject(fw),
    m_form(fw)
{
}

void ResourceHandler::ensureInitialized()
{
    if (m_initialized)
        return;

    m_initialized = true;
    ProjectTree *tree = ProjectTree::instance();

    connect(tree, &ProjectTree::filesAdded, this, &ResourceHandler::updateResources);
    connect(tree, &ProjectTree::filesRemoved, this, &ResourceHandler::updateResources);
    connect(tree, &ProjectTree::foldersAdded, this, &ResourceHandler::updateResources);
    connect(tree, &ProjectTree::foldersRemoved, this, &ResourceHandler::updateResources);
    m_originalUiQrcPaths = m_form->activeResourceFilePaths();
    if (Designer::Constants::Internal::debug)
        qDebug() << "ResourceHandler::ensureInitialized() origPaths=" << m_originalUiQrcPaths;
}

ResourceHandler::~ResourceHandler()
{

}

void ResourceHandler::updateResourcesHelper(bool updateProjectResources)
{
    if (m_handlingResources)
        return;

    ensureInitialized();

    const QString fileName = m_form->fileName();
    QTC_ASSERT(!fileName.isEmpty(), return);

    if (Designer::Constants::Internal::debug)
        qDebug() << "ResourceHandler::updateResources()" << fileName;

    // Filename could change in the meantime.
    Project *project = SessionManager::projectForFile(
                Utils::FileName::fromUserInput(QDir::fromNativeSeparators(fileName)));
    const bool dirty = m_form->property("_q_resourcepathchanged").toBool();
    if (dirty)
        m_form->setDirty(true);

    // Does the file belong to a project?
    if (project) {
        // Collect project resource files.
        ProjectNode *root = project->rootProjectNode();
        QrcFilesVisitor qrcVisitor;
        root->accept(&qrcVisitor);
        QStringList projectQrcFiles = qrcVisitor.qrcFiles();
        // Check if the user has chosen to update the lacking resource inside designer
        if (dirty && updateProjectResources) {
            QStringList qrcPathsToBeAdded;
            foreach (const QString &originalQrcPath, m_originalUiQrcPaths) {
                if (!projectQrcFiles.contains(originalQrcPath) && !qrcPathsToBeAdded.contains(originalQrcPath))
                    qrcPathsToBeAdded.append(originalQrcPath);
            }
            if (!qrcPathsToBeAdded.isEmpty()) {
                m_handlingResources = true;
                root->addFiles(qrcPathsToBeAdded);
                m_handlingResources = false;
                projectQrcFiles += qrcPathsToBeAdded;
            }
        }

        m_form->activateResourceFilePaths(projectQrcFiles);
        m_form->setResourceFileSaveMode(QDesignerFormWindowInterface::SaveOnlyUsedResourceFiles);
        if (Designer::Constants::Internal::debug)
            qDebug() << "ResourceHandler::updateResources()" << fileName
                    << " associated with project" << project->rootProjectNode()->path()
                    <<  " using project qrc files" << projectQrcFiles.size();
    } else {
        // Use resource file originally used in form
        m_form->activateResourceFilePaths(m_originalUiQrcPaths);
        m_form->setResourceFileSaveMode(QDesignerFormWindowInterface::SaveAllResourceFiles);
        if (Designer::Constants::Internal::debug)
            qDebug() << "ResourceHandler::updateResources()" << fileName << " not associated with project, using loaded qrc files.";
    }
}

} // namespace Internal
} // namespace Designer
