/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "resourcehandler.h"
#include "designerconstants.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/nodesvisitor.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#if QT_VERSION >= 0x050000
#    include <QDesignerFormWindowInterface>
#else
#    include "qt_private/formwindowbase_p.h"
#    include "qt_private/qtresourcemodel_p.h"
#endif

#include <utils/qtcassert.h>

using ProjectExplorer::NodesVisitor;
using ProjectExplorer::ProjectNode;
using ProjectExplorer::FolderNode;
using ProjectExplorer::FileNode;

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
        if (fileNode->fileType() == ProjectExplorer::ResourceType)
            m_qrcFiles.append(fileNode->path());
    }
}

// ------------ ResourceHandler
#if QT_VERSION >= 0x050000
ResourceHandler::ResourceHandler(QDesignerFormWindowInterface *fw) :
#else
ResourceHandler::ResourceHandler(qdesigner_internal::FormWindowBase *fw) :
#endif
    QObject(fw),
    m_form(fw),
    m_sessionNode(0),
    m_sessionWatcher(0)
{
}

void ResourceHandler::ensureInitialized()
{
    if (m_sessionNode)
        return;
    ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();
    m_sessionNode = pe->session()->sessionNode();
    m_sessionWatcher = new ProjectExplorer::NodesWatcher();

    connect(m_sessionWatcher, SIGNAL(filesAdded()), this, SLOT(updateResources()));
    connect(m_sessionWatcher, SIGNAL(filesRemoved()), this, SLOT(updateResources()));
    connect(m_sessionWatcher, SIGNAL(foldersAdded()), this, SLOT(updateResources()));
    connect(m_sessionWatcher, SIGNAL(foldersRemoved()), this, SLOT(updateResources()));
    m_sessionNode->registerWatcher(m_sessionWatcher);
#if QT_VERSION >= 0x050000
    m_originalUiQrcPaths = m_form->activeResourceFilePaths();
#else
    m_originalUiQrcPaths = m_form->resourceSet()->activeQrcPaths();
#endif
    if (Designer::Constants::Internal::debug)
        qDebug() << "ResourceHandler::ensureInitialized() origPaths=" << m_originalUiQrcPaths;
}

ResourceHandler::~ResourceHandler()
{
    // Close: Delete the Designer form window via embedding widget
    if (m_sessionNode && m_sessionWatcher) {
        m_sessionNode->unregisterWatcher(m_sessionWatcher);
        delete m_sessionWatcher;
    }
}

void ResourceHandler::updateResources()
{
    ensureInitialized();

    const QString fileName = m_form->fileName();
    QTC_ASSERT(!fileName.isEmpty(), return);

    if (Designer::Constants::Internal::debug)
        qDebug() << "ResourceHandler::updateResources()" << fileName;

    ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();
    // filename could change in the meantime.
    ProjectExplorer::Project *project = pe->session()->projectForFile(fileName);

    // Does the file belong to a project?
    if (project) {
        // Collect project resource files.
        ProjectNode *root = project->rootProjectNode();
        QrcFilesVisitor qrcVisitor;
        root->accept(&qrcVisitor);
        const QStringList projectQrcFiles = qrcVisitor.qrcFiles();
#if QT_VERSION >= 0x050000
        m_form->activateResourceFilePaths(projectQrcFiles);
        m_form->setResourceFileSaveMode(QDesignerFormWindowInterface::SaveOnlyUsedResourceFiles);
#else
        m_form->resourceSet()->activateQrcPaths(projectQrcFiles);
        m_form->setSaveResourcesBehaviour(qdesigner_internal::FormWindowBase::SaveOnlyUsedQrcFiles);
#endif
        if (Designer::Constants::Internal::debug)
            qDebug() << "ResourceHandler::updateResources()" << fileName
                    << " associated with project" << project->rootProjectNode()->path()
                    <<  " using project qrc files" << projectQrcFiles.size();
    } else {
        // Use resource file originally used in form
#if QT_VERSION >= 0x050000
        m_form->activateResourceFilePaths(m_originalUiQrcPaths);
        m_form->setResourceFileSaveMode(QDesignerFormWindowInterface::SaveAllResourceFiles);
#else
        m_form->resourceSet()->activateQrcPaths(m_originalUiQrcPaths);
        m_form->setSaveResourcesBehaviour(qdesigner_internal::FormWindowBase::SaveAll);
#endif
        if (Designer::Constants::Internal::debug)
            qDebug() << "ResourceHandler::updateResources()" << fileName << " not associated with project, using loaded qrc files.";
    }
}

} // namespace Internal
} // namespace Designer
