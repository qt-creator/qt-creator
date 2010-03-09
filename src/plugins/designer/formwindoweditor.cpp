/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "formwindoweditor.h"
#include "designerconstants.h"
#include "formeditorw.h"

#include <coreplugin/ifile.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/nodesvisitor.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <utils/qtcassert.h>

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowManagerInterface>
#include <QtDesigner/QDesignerPropertyEditorInterface>
#include "qt_private/formwindowbase_p.h"
#include "qt_private/qtresourcemodel_p.h"

#include <QtCore/QDebug>

using namespace Designer;
using namespace Designer::Internal;
using namespace Designer::Constants;
using namespace SharedTools;
using ProjectExplorer::NodesVisitor;
using ProjectExplorer::ProjectNode;
using ProjectExplorer::FolderNode;
using ProjectExplorer::FileNode;

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


FormWindowEditor::FormWindowEditor(QDesignerFormWindowInterface *form,
                                   QWidget *parent) :
    SharedTools::WidgetHost(parent, form),
    m_file(0),
    m_sessionNode(0),
    m_sessionWatcher(0)
{
    connect(this, SIGNAL(formWindowSizeChanged(int,int)), this, SLOT(slotFormSizeChanged(int,int)));
}

void FormWindowEditor::setFile(Core::IFile *file)
{
     m_file = file;
}

FormWindowEditor::~FormWindowEditor()
{
    // Close: Delete the Designer form window via embedding widget
    if (m_sessionNode && m_sessionWatcher) {
        m_sessionNode->unregisterWatcher(m_sessionWatcher);
        delete m_sessionWatcher;
    }
}

void FormWindowEditor::initializeResources(const QString & /* fileName */)
{
    ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::SessionManager *session = pe->session();

    m_sessionNode = session->sessionNode();
    m_sessionWatcher = new ProjectExplorer::NodesWatcher();

    connect(m_sessionWatcher, SIGNAL(filesAdded()), this, SLOT(updateResources()));
    connect(m_sessionWatcher, SIGNAL(filesRemoved()), this, SLOT(updateResources()));
    connect(m_sessionWatcher, SIGNAL(foldersAdded()), this, SLOT(updateResources()));
    connect(m_sessionWatcher, SIGNAL(foldersRemoved()), this, SLOT(updateResources()));
    m_sessionNode->registerWatcher(m_sessionWatcher);

    if (qdesigner_internal::FormWindowBase *fw = qobject_cast<qdesigner_internal::FormWindowBase *>(formWindow())) {
        QtResourceSet *rs = fw->resourceSet();
        m_originalUiQrcPaths = rs->activeQrcPaths();
    }

    updateResources();
}

void FormWindowEditor::updateResources()
{
    if (qdesigner_internal::FormWindowBase *fw = qobject_cast<qdesigner_internal::FormWindowBase *>(formWindow())) {
        ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();
        // filename could change in the meantime.
        ProjectExplorer::Project *project = pe->session()->projectForFile(m_file->fileName());

        qdesigner_internal::FormWindowBase::SaveResourcesBehaviour behaviour = qdesigner_internal::FormWindowBase::SaveAll;
        QtResourceSet *rs = fw->resourceSet();
        if (project) {
            ProjectNode *root = project->rootProjectNode();
            QrcFilesVisitor qrcVisitor;
            root->accept(&qrcVisitor);

            rs->activateQrcPaths(qrcVisitor.qrcFiles());
            behaviour = qdesigner_internal::FormWindowBase::SaveOnlyUsedQrcFiles;
        } else {
            rs->activateQrcPaths(m_originalUiQrcPaths);
        }
        fw->setSaveResourcesBehaviour(behaviour);
    }
}

Core::IFile *FormWindowEditor::file() const
{
    return m_file;
}

void FormWindowEditor::slotFormSizeChanged(int w, int h)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << w << h;

    formWindow()->setDirty(true);
    static const QString geometry = QLatin1String("geometry");
    FormEditorW::instance()->designerEditor()->propertyEditor()->setPropertyValue(geometry, QRect(0,0,w,h) );
}
