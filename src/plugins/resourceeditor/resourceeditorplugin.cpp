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

#include "resourceeditorplugin.h"

#include "resourceeditorw.h"
#include "resourceeditorconstants.h"
#include "resourceeditorfactory.h"
#include "resourcenode.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/id.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/mimetypes/mimedatabase.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>

#include <QtPlugin>
#include <QCoreApplication>
#include <QAction>
#include <QDebug>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QClipboard>
#include <QApplication>

using namespace ProjectExplorer;

namespace ResourceEditor {
namespace Internal {

static const char resourcePrefix[] = ":";
static const char urlPrefix[] = "qrc:";

class PrefixLangDialog : public QDialog
{
    Q_OBJECT
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
                                                         Qt::Horizontal,
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
    Q_UNUSED(errorMessage)
    Utils::MimeDatabase::addMimeTypes(QLatin1String(":/resourceeditor/ResourceEditor.mimetypes.xml"));

    ResourceEditorFactory *editor = new ResourceEditorFactory(this);
    addAutoReleasedObject(editor);

    // Register undo and redo
    const Core::Context context(Constants::C_RESOURCEEDITOR);
    m_undoAction = new QAction(tr("&Undo"), this);
    m_redoAction = new QAction(tr("&Redo"), this);
    m_refreshAction = new QAction(tr("Recheck Existence of Referenced Files"), this);
    Core::ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, context);
    Core::ActionManager::registerAction(m_redoAction, Core::Constants::REDO, context);
    Core::ActionManager::registerAction(m_refreshAction, Constants::REFRESH, context);
    connect(m_undoAction, SIGNAL(triggered()), this, SLOT(onUndo()));
    connect(m_redoAction, SIGNAL(triggered()), this, SLOT(onRedo()));
    connect(m_refreshAction, SIGNAL(triggered()), this, SLOT(onRefresh()));

    Core::Context projectTreeContext(ProjectExplorer::Constants::C_PROJECT_TREE);
    Core::ActionContainer *folderContextMenu =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_FOLDERCONTEXT);
    Core::ActionContainer *fileContextMenu =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_FILECONTEXT);
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

    m_removeNonExisting = new QAction(tr("Remove Missing Files"), this);
    command = Core::ActionManager::registerAction(m_removeNonExisting, Constants::C_REMOVE_NON_EXISTING, projectTreeContext);
    folderContextMenu->addAction(command, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(m_removeNonExisting, &QAction::triggered, this, &ResourceEditorPlugin::removeNonExisting);

    m_renameResourceFile = new QAction(tr("Rename..."), this);
    command = Core::ActionManager::registerAction(m_renameResourceFile, Constants::C_RENAME_FILE, projectTreeContext);
    folderContextMenu->addAction(command, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(m_renameResourceFile, SIGNAL(triggered()), this, SLOT(renameFileContextMenu()));

    m_removeResourceFile = new QAction(tr("Remove File..."), this);
    command = Core::ActionManager::registerAction(m_removeResourceFile, Constants::C_REMOVE_FILE, projectTreeContext);
    folderContextMenu->addAction(command, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(m_removeResourceFile, SIGNAL(triggered()), this, SLOT(removeFileContextMenu()));

    m_openInEditor = new QAction(tr("Open in Editor"), this);
    command = Core::ActionManager::registerAction(m_openInEditor, Constants::C_OPEN_EDITOR, projectTreeContext);
    folderContextMenu->addAction(command, ProjectExplorer::Constants::G_FOLDER_FILES);
    connect(m_openInEditor, SIGNAL(triggered()), this, SLOT(openEditorContextMenu()));

    m_openWithMenu = new QMenu(tr("Open With"), folderContextMenu->menu());
    folderContextMenu->menu()->insertMenu(
                folderContextMenu->insertLocation(ProjectExplorer::Constants::G_FOLDER_FILES),
                m_openWithMenu);

    m_copyPath = new Utils::ParameterAction(QString(), tr("Copy path \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_copyPath, Constants::C_COPY_PATH, projectTreeContext);
    command->setAttribute(Core::Command::CA_UpdateText);
    fileContextMenu->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(m_copyPath, SIGNAL(triggered()), this, SLOT(copyPathContextMenu()));

    m_copyUrl = new Utils::ParameterAction(QString(), tr("Copy url \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_copyUrl, Constants::C_COPY_URL, projectTreeContext);
    command->setAttribute(Core::Command::CA_UpdateText);
    fileContextMenu->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(m_copyUrl, SIGNAL(triggered()), this, SLOT(copyUrlContextMenu()));

    m_addPrefix->setEnabled(false);
    m_removePrefix->setEnabled(false);
    m_renamePrefix->setEnabled(false);
    m_removeNonExisting->setEnabled(false);
    m_renameResourceFile->setEnabled(false);
    m_removeResourceFile->setEnabled(false);

    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
            this, &ResourceEditorPlugin::updateContextActions);

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
    auto topLevel = static_cast<ResourceTopLevelNode *>(ProjectTree::currentNode());
    PrefixLangDialog dialog(tr("Add Prefix"), QString(), QString(), Core::ICore::mainWindow());
    if (dialog.exec() != QDialog::Accepted)
        return;
    QString prefix = dialog.prefix();
    if (prefix.isEmpty())
        return;
    topLevel->addPrefix(prefix, dialog.lang());
}

void ResourceEditorPlugin::removePrefixContextMenu()
{
    ResourceFolderNode *rfn = static_cast<ResourceFolderNode *>(ProjectTree::currentNode());
    if (QMessageBox::question(Core::ICore::mainWindow(),
                              tr("Remove Prefix"),
                              tr("Remove prefix %1 and all its files?").arg(rfn->displayName()))
            == QMessageBox::Yes) {
        ResourceTopLevelNode *rn = rfn->resourceNode();
        rn->removePrefix(rfn->prefix(), rfn->lang());
    }
}

void ResourceEditorPlugin::removeNonExisting()
{
    ResourceTopLevelNode *topLevel = static_cast<ResourceTopLevelNode *>(ProjectTree::currentNode());
    topLevel->removeNonExistingFiles();
}

void ResourceEditorPlugin::renameFileContextMenu()
{
    ProjectExplorerPlugin::initiateInlineRenaming();
}

void ResourceEditorPlugin::removeFileContextMenu()
{
    ResourceFolderNode *rfn = static_cast<ResourceFolderNode *>(ProjectTree::currentNode());
    QString path = rfn->path().toString();
    FolderNode *parent = rfn->parentFolderNode();
    if (!parent->removeFiles(QStringList() << path))
        QMessageBox::warning(Core::ICore::mainWindow(),
                             tr("File Removal Failed"),
                             tr("Removing file %1 from the project failed.").arg(path));
}

void ResourceEditorPlugin::openEditorContextMenu()
{
    Core::EditorManager::openEditor(ProjectTree::currentNode()->path().toString());
}

void ResourceEditorPlugin::copyPathContextMenu()
{
    ResourceFileNode *node = static_cast<ResourceFileNode *>(ProjectTree::currentNode());
    QApplication::clipboard()->setText(QLatin1String(resourcePrefix) + node->qrcPath());
}

void ResourceEditorPlugin::copyUrlContextMenu()
{
    ResourceFileNode *node = static_cast<ResourceFileNode *>(ProjectTree::currentNode());
    QApplication::clipboard()->setText(QLatin1String(urlPrefix) + node->qrcPath());
}

void ResourceEditorPlugin::renamePrefixContextMenu()
{
    ResourceFolderNode *node = static_cast<ResourceFolderNode *>(ProjectTree::currentNode());

    PrefixLangDialog dialog(tr("Rename Prefix"), node->prefix(), node->lang(), Core::ICore::mainWindow());
    if (dialog.exec() != QDialog::Accepted)
        return;
    QString prefix = dialog.prefix();
    if (prefix.isEmpty())
        return;

    node->renamePrefix(prefix, dialog.lang());
}

void ResourceEditorPlugin::updateContextActions(Node *node, Project *)
{
    bool isResourceNode = dynamic_cast<ResourceTopLevelNode *>(node);
    m_addPrefix->setEnabled(isResourceNode);
    m_addPrefix->setVisible(isResourceNode);

    bool enableRename = false;
    bool enableRemove = false;

    if (isResourceNode) {
        FolderNode *parent = node ? node->parentFolderNode() : 0;
        enableRename = parent && parent->supportedActions(node).contains(Rename);
        enableRemove = parent && parent->supportedActions(node).contains(RemoveFile);
    }

    m_renameResourceFile->setEnabled(isResourceNode && enableRename);
    m_renameResourceFile->setVisible(isResourceNode && enableRename);
    m_removeResourceFile->setEnabled(isResourceNode && enableRemove);
    m_removeResourceFile->setVisible(isResourceNode && enableRemove);

    m_openInEditor->setEnabled(isResourceNode);
    m_openInEditor->setVisible(isResourceNode);

    bool isResourceFolder = dynamic_cast<ResourceFolderNode *>(node);
    m_removePrefix->setEnabled(isResourceFolder);
    m_removePrefix->setVisible(isResourceFolder);

    m_renamePrefix->setEnabled(isResourceFolder);
    m_renamePrefix->setVisible(isResourceFolder);

    m_removeNonExisting->setEnabled(isResourceNode);
    m_removeNonExisting->setVisible(isResourceNode);

    if (isResourceNode)
        Core::EditorManager::populateOpenWithMenu(m_openWithMenu, node->path().toString());
    else
        m_openWithMenu->clear();
    m_openWithMenu->menuAction()->setVisible(!m_openWithMenu->actions().isEmpty());

    bool isResourceFile = dynamic_cast<ResourceFileNode *>(node);
    m_copyPath->setEnabled(isResourceFile);
    m_copyPath->setVisible(isResourceFile);
    m_copyUrl->setEnabled(isResourceFile);
    m_copyUrl->setVisible(isResourceFile);
    if (isResourceFile) {
        ResourceFileNode *fileNode = static_cast<ResourceFileNode *>(node);
        QString qrcPath = fileNode->qrcPath();
        m_copyPath->setParameter(QLatin1String(resourcePrefix) + qrcPath);
        m_copyUrl->setParameter(QLatin1String(urlPrefix) + qrcPath);
    }
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

} // namespace Internal
} // namespace ResourceEditor

#include "resourceeditorplugin.moc"
