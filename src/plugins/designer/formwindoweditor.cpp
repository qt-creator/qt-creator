/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "designerconstants.h"
#include "editorwidget.h"
#include "formeditorw.h"
#include "formwindoweditor.h"
#include "formwindowfile.h"
#include "formwindowhost.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/nodesvisitor.h>

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowManagerInterface>
#include <QtDesigner/QDesignerPropertyEditorInterface>
#include <QtDesigner/QDesignerWidgetDataBaseInterface>
#include <qt_private/formwindowbase_p.h>
#include <qt_private/qtresourcemodel_p.h>

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QByteArray>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>
#include <QtCore/QDebug>
#include <QtGui/QToolBar>

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


FormWindowEditor::FormWindowEditor(const QList<int> &context,
                                   QDesignerFormWindowInterface *form,
                                   QObject *parent)
  : Core::IEditor(parent),
    m_context(context),
    m_formWindow(form),
    m_file(new FormWindowFile(form, this)),
    m_host(new FormWindowHost(form)),
    m_editorWidget(new EditorWidget(m_host)),
    m_toolBar(0),
    m_sessionNode(0),
    m_sessionWatcher(0)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << form << parent;

    connect(m_file, SIGNAL(reload(QString)), this, SLOT(slotOpen(QString)));
    connect(m_file, SIGNAL(setDisplayName(QString)), this, SLOT(slotSetDisplayName(QString)));
    connect(m_file, SIGNAL(changed()), this, SIGNAL(changed()));
    connect(m_file, SIGNAL(changed()), this, SLOT(updateResources()));
    connect(this, SIGNAL(opened(QString)), m_file, SLOT(setFileName(QString)));

    connect(m_host, SIGNAL(changed()), this, SIGNAL(changed()));

    connect(form, SIGNAL(toolChanged(int)), m_editorWidget, SLOT(toolChanged(int)));
    m_editorWidget->activate();
}

FormWindowEditor::~FormWindowEditor()
{
    // Close: Delete the Designer form window via embedding widget
    delete m_toolBar;
    delete m_host;
    delete m_editorWidget;
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << m_displayName;
    if (m_sessionNode && m_sessionWatcher) {
        m_sessionNode->unregisterWatcher(m_sessionWatcher);
        delete m_sessionWatcher;
    }
}

bool FormWindowEditor::createNew(const QString &contents)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << contents.size() << "chars";

    if (!m_formWindow)
        return false;

    m_formWindow->setContents(contents);
    if (!m_formWindow->mainContainer())
        return false;

    if (qdesigner_internal::FormWindowBase *fw = qobject_cast<qdesigner_internal::FormWindowBase *>(m_formWindow))
        fw->setDesignerGrid(qdesigner_internal::FormWindowBase::defaultDesignerGrid());
    return true;
}

bool FormWindowEditor::open(const QString &fileName /*= QString()*/)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << fileName;

    if (fileName.isEmpty()) {
        setDisplayName(tr("untitled"));
    } else {
        const QFileInfo fi(fileName);
        const QString fileName = fi.absoluteFilePath();

        QFile file(fileName);
        if (!file.exists())
            return false;

        if (!fi.isReadable())
            return false;

        if (!file.open(QIODevice::ReadOnly|QIODevice::Text))
            return false;

        m_formWindow->setFileName(fileName);
        m_formWindow->setContents(&file);
        file.close();
        if (!m_formWindow->mainContainer())
            return false;
        m_formWindow->setDirty(false);

        ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();
        ProjectExplorer::SessionManager *session = pe->session();
        m_sessionNode = session->sessionNode();
        m_sessionWatcher = new ProjectExplorer::NodesWatcher();
        connect(m_sessionWatcher, SIGNAL(filesAdded()), this, SLOT(updateResources()));
        connect(m_sessionWatcher, SIGNAL(filesRemoved()), this, SLOT(updateResources()));
        connect(m_sessionWatcher, SIGNAL(foldersAdded()), this, SLOT(updateResources()));
        connect(m_sessionWatcher, SIGNAL(foldersRemoved()), this, SLOT(updateResources()));
        m_sessionNode->registerWatcher(m_sessionWatcher);

        if (qdesigner_internal::FormWindowBase *fw = qobject_cast<qdesigner_internal::FormWindowBase *>(m_formWindow)) {
            QtResourceSet *rs = fw->resourceSet();
            m_originalUiQrcPaths = rs->activeQrcPaths();
        }

        emit opened(fileName);
        updateResources();

        QDesignerFormWindowManagerInterface *fwm = FormEditorW::instance()->designerEditor()->formWindowManager();
        fwm->setActiveFormWindow(m_formWindow);

        setDisplayName(fi.fileName());
    }
    emit changed();
    return true;
}

void FormWindowEditor::updateResources()
{
    if (qdesigner_internal::FormWindowBase *fw = qobject_cast<qdesigner_internal::FormWindowBase *>(m_formWindow)) {
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

void FormWindowEditor::slotOpen(const QString &fileName)
{
    open(fileName);
}

void FormWindowEditor::slotSetDisplayName(const QString &title)
{
    if (Designer::Constants::Internal::debug)
        qDebug() <<  Q_FUNC_INFO << title;
    setDisplayName(title);
}

bool FormWindowEditor::duplicateSupported() const
{
    return false;
}

Core::IEditor *FormWindowEditor::duplicate(QWidget *)
{
    return 0;
}

Core::IFile *FormWindowEditor::file()
{
    return m_file;
}

const char *FormWindowEditor::kind() const
{
    return C_FORMEDITOR;
}

QString FormWindowEditor::displayName() const
{
    return m_displayName;
}

void FormWindowEditor::setDisplayName(const QString &title)
{
    m_displayName = title;
}

QToolBar *FormWindowEditor::toolBar()
{
    if (!m_toolBar)
        m_toolBar = FormEditorW::instance()->createEditorToolBar();
    return m_toolBar;
}

QByteArray FormWindowEditor::saveState() const
{
    return QByteArray();
}

bool FormWindowEditor::restoreState(const QByteArray &/*state*/)
{
    return true;
}

QList<int> FormWindowEditor::context() const
{
    return m_context;
}

QWidget *FormWindowEditor::widget()
{
    return m_editorWidget;
}


QDesignerFormWindowInterface *FormWindowEditor::formWindow() const
{
    return m_formWindow;
}

QWidget *FormWindowEditor::integrationContainer()
{
    return m_host->integrationContainer();
}

void FormWindowEditor::updateFormWindowSelectionHandles(bool state)
{
    m_host->updateFormWindowSelectionHandles(state);
}

void FormWindowEditor::setSuggestedFileName(const QString &fileName)
{
    m_file->setSuggestedFileName(fileName);
}

void FormWindowEditor::activate()
{
    m_editorWidget->activate();
}

QString FormWindowEditor::contextHelpId() const
{
    // TODO [13.2.09]: Replace this by QDesignerIntegrations context help Id
    // in the upcoming version of Qt
    QDesignerFormEditorInterface *core = FormEditorW::instance()->designerEditor();
    QObject *o = core->propertyEditor()->object();
    if (!o)
        return QString();
    const QDesignerWidgetDataBaseInterface *db = core->widgetDataBase();
    const int dbIndex = db->indexOfObject(o, true);
    if (dbIndex == -1)
        return QString();
    QString className = db->item(dbIndex)->name();
    if (className == QLatin1String("Line"))
        className = QLatin1String("QFrame");
    else if (className == QLatin1String("Spacer"))
        className = QLatin1String("QSpacerItem");
    else if (className == QLatin1String("QLayoutWidget"))
        className = QLatin1String("QLayout");
    return className;
}
