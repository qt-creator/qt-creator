/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "resourceeditorplugin.h"

#include "resourceeditorw.h"
#include "resourceeditorconstants.h"
#include "resourcewizard.h"
#include "resourceeditorfactory.h"
#include "resourcenode.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/id.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QtPlugin>
#include <QCoreApplication>
#include <QAction>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QDialogButtonBox>

using namespace ResourceEditor::Internal;

class PrefixLangDialog : public QDialog
{
public:
    PrefixLangDialog(const QString &title, const QString &prefix, const QString &lang, QWidget *parent)
        : QDialog(parent)
    {
        setWindowTitle(title);
        QFormLayout *layout = new QFormLayout(this);
        m_prefixLineEdit = new QLineEdit(this);
        m_prefixLineEdit->setText(prefix);
        layout->addRow(tr("Prefix:"), m_prefixLineEdit);

        m_langLineEdit = new QLineEdit(this);
        m_langLineEdit->setText(lang);
        layout->addRow(tr("Language:"), m_langLineEdit);

        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                                         | QDialogButtonBox::Cancel,
                                                         this);

        layout->addWidget(buttons);

        connect(buttons, SIGNAL(accepted()),
                this, SLOT(accept()));
        connect(buttons, SIGNAL(rejected()),
                this, SLOT(reject()));
    }
    QString prefix() const
    {
        return m_prefixLineEdit->text();
    }

    QString lang() const
    {
        return m_langLineEdit->text();
    }
private:
    QLineEdit *m_prefixLineEdit;
    QLineEdit *m_langLineEdit;
};

ResourceEditorPlugin::ResourceEditorPlugin() :
    m_redoAction(0),
    m_undoAction(0)
{
}

bool ResourceEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    if (!Core::MimeDatabase::addMimeTypes(QLatin1String(":/resourceeditor/ResourceEditor.mimetypes.xml"), errorMessage))
        return false;

    ResourceEditorFactory *editor = new ResourceEditorFactory(this);
    addAutoReleasedObject(editor);

    ResourceWizard *wizard = new ResourceWizard;
    wizard->setDescription(tr("Creates a Qt Resource file (.qrc) that you can add to a Qt Widget Project."));
    wizard->setDisplayName(tr("Qt Resource file"));
    wizard->setId(QLatin1String("F.Resource"));
    wizard->setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    wizard->setDisplayCategory(QCoreApplication::translate("Core", Core::Constants::WIZARD_TR_CATEGORY_QT));

    addAutoReleasedObject(wizard);

    errorMessage->clear();

    // Register undo and redo
    const Core::Context context(Constants::C_RESOURCEEDITOR);
    m_undoAction = new QAction(tr("&Undo"), this);
    m_redoAction = new QAction(tr("&Redo"), this);
    m_refreshAction = new QAction(tr("Recheck existence of referenced files"), this);
    Core::ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, context);
    Core::ActionManager::registerAction(m_redoAction, Core::Constants::REDO, context);
    Core::ActionManager::registerAction(m_refreshAction, Constants::REFRESH, context);
    connect(m_undoAction, SIGNAL(triggered()), this, SLOT(onUndo()));
    connect(m_redoAction, SIGNAL(triggered()), this, SLOT(onRedo()));
    connect(m_refreshAction, SIGNAL(triggered()), this, SLOT(onRefresh()));

    Core::Context projectTreeContext(ProjectExplorer::Constants::C_PROJECT_TREE);
    Core::ActionContainer *folderContextMenu =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_FOLDERCONTEXT);
    Core::Command *command = 0;

    m_addPrefix = new QAction(tr("Add Prefix..."), this);
    command = Core::ActionManager::registerAction(m_addPrefix, Constants::C_ADD_PREFIX, projectTreeContext);
    folderContextMenu->addAction(command, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(m_addPrefix, SIGNAL(triggered()), this, SLOT(addPrefixContextMenu()));

    m_renamePrefix = new QAction(tr("Change Prefix..."), this);
    command = Core::ActionManager::registerAction(m_renamePrefix, Constants::C_RENAME_PREFIX, projectTreeContext);
    folderContextMenu->addAction(command, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(m_renamePrefix, SIGNAL(triggered()), this, SLOT(renamePrefixContextMenu()));

    m_removePrefix = new QAction(tr("Remove Prefix..."), this);
    command = Core::ActionManager::registerAction(m_removePrefix, Constants::C_REMOVE_PREFIX, projectTreeContext);
    folderContextMenu->addAction(command, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(m_removePrefix, SIGNAL(triggered()), this, SLOT(removePrefixContextMenu()));

    m_renameResourceFile = new QAction(tr("Rename File"), this);
    command = Core::ActionManager::registerAction(m_renameResourceFile, Constants::C_RENAME_FILE, projectTreeContext);
    folderContextMenu->addAction(command, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(m_renameResourceFile, SIGNAL(triggered()), this, SLOT(renameFileContextMenu()));

    m_removeResourceFile = new QAction(tr("Remove File"), this);
    command = Core::ActionManager::registerAction(m_removeResourceFile, Constants::C_REMOVE_FILE, projectTreeContext);
    folderContextMenu->addAction(command, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(m_removeResourceFile, SIGNAL(triggered()), this, SLOT(removeFileContextMenu()));

    m_openInEditor = new QAction(tr("Open in Editor"), this);
    command = Core::ActionManager::registerAction(m_openInEditor, Constants::C_OPEN_EDITOR, projectTreeContext);
    folderContextMenu->addAction(command, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(m_openInEditor, SIGNAL(triggered()), this, SLOT(openEditorContextMenu()));

    m_openInTextEditor = new QAction(tr("Open in Text Editor"), this);
    command = Core::ActionManager::registerAction(m_openInTextEditor, Constants::C_OPEN_TEXT_EDITOR, projectTreeContext);
    folderContextMenu->addAction(command, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(m_openInTextEditor, SIGNAL(triggered()), this, SLOT(openTextEditorContextMenu()));

    m_addPrefix->setEnabled(false);
    m_removePrefix->setEnabled(false);
    m_renamePrefix->setEnabled(false);
    m_renameResourceFile->setEnabled(false);
    m_removeResourceFile->setEnabled(false);

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(currentNodeChanged(ProjectExplorer::Node*,ProjectExplorer::Project*)),
            this, SLOT(updateContextActions(ProjectExplorer::Node*,ProjectExplorer::Project*)));


    return true;
}

void ResourceEditorPlugin::extensionsInitialized()
{
}

void ResourceEditorPlugin::onUndo()
{
    currentEditor()->onUndo();
}

void ResourceEditorPlugin::onRedo()
{
    currentEditor()->onRedo();
}

void ResourceEditorPlugin::onRefresh()
{
    currentEditor()->onRefresh();
}

void ResourceEditorPlugin::addPrefixContextMenu()
{
    PrefixLangDialog dialog(tr("Add Prefix"), QString(), QString(), Core::ICore::mainWindow());
    if (dialog.exec() != QDialog::Accepted)
        return;
    QString prefix = dialog.prefix();
    if (prefix.isEmpty())
        return;
    ResourceTopLevelNode *topLevel = static_cast<ResourceTopLevelNode *>(ProjectExplorer::ProjectExplorerPlugin::instance()->currentNode());
    topLevel->addPrefix(prefix, dialog.lang());
}

void ResourceEditorPlugin::removePrefixContextMenu()
{
    ResourceFolderNode *rfn = static_cast<ResourceFolderNode *>(ProjectExplorer::ProjectExplorerPlugin::instance()->currentNode());
    if (QMessageBox::question(Core::ICore::mainWindow(),
                              tr("Remove Prefix"),
                              tr("Remove prefix %1 and all its files?").arg(rfn->displayName()))
            == QMessageBox::Yes) {
        ResourceTopLevelNode *rn = rfn->resourceNode();
        rn->removePrefix(rfn->prefix(), rfn->lang());
    }
}

void ResourceEditorPlugin::renameFileContextMenu()
{
    ProjectExplorer::ProjectExplorerPlugin::instance()->initiateInlineRenaming();
}

void ResourceEditorPlugin::removeFileContextMenu()
{
    ResourceFolderNode *rfn = static_cast<ResourceFolderNode *>(ProjectExplorer::ProjectExplorerPlugin::instance()->currentNode());
    QString path = rfn->path();
    ProjectExplorer::FolderNode *parent = rfn->parentFolderNode();
    if (!parent->removeFiles(QStringList() << path))
        QMessageBox::warning(Core::ICore::mainWindow(),
                             tr("File Removal failed"),
                             tr("Removing file %1 from the project failed.").arg(path));
}

void ResourceEditorPlugin::openEditorContextMenu()
{
    ResourceTopLevelNode *topLevel = static_cast<ResourceTopLevelNode *>(ProjectExplorer::ProjectExplorerPlugin::instance()->currentNode());
    QString path = topLevel->path();
    Core::EditorManager::openEditor(path);
}

void ResourceEditorPlugin::openTextEditorContextMenu()
{
    ResourceTopLevelNode *topLevel = static_cast<ResourceTopLevelNode *>(ProjectExplorer::ProjectExplorerPlugin::instance()->currentNode());
    QString path = topLevel->path();
    Core::EditorManager::openEditor(path, Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
}

void ResourceEditorPlugin::renamePrefixContextMenu()
{
    ResourceFolderNode *rfn = static_cast<ResourceFolderNode *>(ProjectExplorer::ProjectExplorerPlugin::instance()->currentNode());

    PrefixLangDialog dialog(tr("Rename Prefix"), rfn->prefix(), rfn->lang(), Core::ICore::mainWindow());
    if (dialog.exec() != QDialog::Accepted)
        return;
    QString prefix = dialog.prefix();
    if (prefix.isEmpty())
        return;

    rfn->renamePrefix(prefix, dialog.lang());
}

void ResourceEditorPlugin::updateContextActions(ProjectExplorer::Node *node, ProjectExplorer::Project *)
{
    bool isResourceNode = qobject_cast<ResourceTopLevelNode *>(node);
    m_addPrefix->setEnabled(isResourceNode);
    m_addPrefix->setVisible(isResourceNode);

    bool enableRename = false;
    bool enableRemove = false;

    if (isResourceNode) {
        ProjectExplorer::FolderNode *parent = node ? node->parentFolderNode() : 0;
        enableRename = parent && parent->supportedActions(node).contains(ProjectExplorer::Rename);
        enableRemove = parent && parent->supportedActions(node).contains(ProjectExplorer::RemoveFile);
    }

    m_renameResourceFile->setEnabled(isResourceNode && enableRename);
    m_renameResourceFile->setVisible(isResourceNode && enableRename);
    m_removeResourceFile->setEnabled(isResourceNode && enableRemove);
    m_removeResourceFile->setVisible(isResourceNode && enableRemove);

    m_openInEditor->setEnabled(isResourceNode);
    m_openInEditor->setVisible(isResourceNode);

    m_openInTextEditor->setEnabled(isResourceNode);
    m_openInTextEditor->setVisible(isResourceNode);

    bool isResourceFolder = qobject_cast<ResourceFolderNode *>(node);
    m_removePrefix->setEnabled(isResourceFolder);
    m_removePrefix->setVisible(isResourceFolder);

    m_renamePrefix->setEnabled(isResourceFolder);
    m_renamePrefix->setVisible(isResourceFolder);
}

void ResourceEditorPlugin::onUndoStackChanged(ResourceEditorW const *editor,
        bool canUndo, bool canRedo)
{
    if (editor == currentEditor()) {
        m_undoAction->setEnabled(canUndo);
        m_redoAction->setEnabled(canRedo);
    }
}

ResourceEditorW * ResourceEditorPlugin::currentEditor() const
{
    ResourceEditorW * const focusEditor = qobject_cast<ResourceEditorW *>(
            Core::EditorManager::currentEditor());
    QTC_ASSERT(focusEditor, return 0);
    return focusEditor;
}

Q_EXPORT_PLUGIN(ResourceEditorPlugin)
