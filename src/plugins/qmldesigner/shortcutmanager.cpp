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

#include "shortcutmanager.h"

#include "designersettings.h"

#include <viewmanager.h>
#include <designeractionmanagerview.h>
#include <componentcore_constants.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljseditor/qmljseditordocument.h>

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/proxyaction.h>
#include <utils/utilsicons.h>
#include <utils/qtcassert.h>

#include <qmljs/qmljsreformatter.h>

#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "designmodewidget.h"

#include <QApplication>
#include <QClipboard>

static void updateClipboard(QAction *action)
{
    const bool dataInClipboard = !QApplication::clipboard()->text().isEmpty();
    action->setEnabled(dataInClipboard);
}

namespace QmlDesigner {

ShortCutManager::ShortCutManager()
    : QObject(),
    m_revertToSavedAction(0),
    m_saveAction(0),
    m_saveAsAction(0),
    m_exportAsImageAction(tr("Export as &Image..."), 0),
    m_closeCurrentEditorAction(0),
    m_closeAllEditorsAction(0),
    m_closeOtherEditorsAction(0),
    m_undoAction(tr("&Undo"), 0),
    m_redoAction(tr("&Redo"), 0),
    m_deleteAction(tr("Delete"), tr("Delete \"%1\""), Utils::ParameterAction::EnabledWithParameter),
    m_cutAction(tr("Cu&t"), tr("Cut \"%1\""), Utils::ParameterAction::EnabledWithParameter),
    m_copyAction(tr("&Copy"), tr("Copy \"%1\""), Utils::ParameterAction::EnabledWithParameter),
    m_pasteAction(tr("&Paste"), tr("Paste \"%1\""), Utils::ParameterAction::EnabledWithParameter),
    m_selectAllAction(tr("Select &All"), tr("Select All \"%1\""), Utils::ParameterAction::EnabledWithParameter),
    m_collapseExpandStatesAction(tr("Toggle States Editor"), 0),
    m_restoreDefaultViewAction(tr("&Restore Default View"), 0),
    m_toggleLeftSidebarAction(tr("Toggle &Left Sidebar"), 0),
    m_toggleRightSidebarAction(tr("Toggle &Right Sidebar"), 0),
    m_switchTextFormAction(tr("Switch Text/Design"), 0),
    m_escapeAction(this)
{

}

void ShortCutManager::registerActions(const Core::Context &qmlDesignerMainContext,
                                      const Core::Context &qmlDesignerFormEditorContext,
                                      const Core::Context &qmlDesignerNavigatorContext)
{
    Core::ActionContainer *editMenu = Core::ActionManager::actionContainer(Core::Constants::M_EDIT);
    Core::ActionContainer *fileMenu = Core::ActionManager::actionContainer(Core::Constants::M_FILE);

    connect(&m_undoAction, &QAction::triggered, this, &ShortCutManager::undo);

    connect(&m_redoAction, &QAction::triggered, this, &ShortCutManager::redo);

    connect(&m_deleteAction, &QAction::triggered, this, &ShortCutManager::deleteSelected);

    connect(&m_cutAction, &QAction::triggered, this, &ShortCutManager::cutSelected);

    connect(&m_copyAction, &QAction::triggered, this, &ShortCutManager::copySelected);

    connect(&m_pasteAction, &QAction::triggered, this, &ShortCutManager::paste);

    connect(&m_selectAllAction,&QAction::triggered, this, &ShortCutManager::selectAll);

    connect(&m_restoreDefaultViewAction,
            &QAction::triggered,
            QmlDesignerPlugin::instance()->mainWidget(),
            &Internal::DesignModeWidget::restoreDefaultView);

    connect(&m_toggleLeftSidebarAction,
            &QAction::triggered,
            QmlDesignerPlugin::instance()->mainWidget(),
            &Internal::DesignModeWidget::toggleLeftSidebar);

    connect(&m_toggleRightSidebarAction,
            &QAction::triggered,
            QmlDesignerPlugin::instance()->mainWidget(),
            &Internal::DesignModeWidget::toggleRightSidebar);

    connect(&m_switchTextFormAction,
            &QAction::triggered,
            QmlDesignerPlugin::instance()->mainWidget(),
            &Internal::DesignModeWidget::switchTextOrForm);

    connect(&m_collapseExpandStatesAction, &QAction::triggered, [] {
        QmlDesignerPlugin::instance()->viewManager().toggleStatesViewExpanded();
    });

    // Revert to saved
    Core::EditorManager *em = Core::EditorManager::instance();
    Core::ActionManager::registerAction(&m_revertToSavedAction,Core::Constants::REVERTTOSAVED, qmlDesignerMainContext);
    connect(&m_revertToSavedAction, &QAction::triggered, em, &Core::EditorManager::revertToSaved);

    //Save
    Core::ActionManager::registerAction(&m_saveAction, Core::Constants::SAVE, qmlDesignerMainContext);
    connect(&m_saveAction, &QAction::triggered, em, [em] {
         QmlDesignerPlugin::instance()->viewManager().reformatFileUsingTextEditorView();
         em->saveDocument();
    });

    Core::Command *command = nullptr;

    //Save As
    Core::ActionManager::registerAction(&m_saveAsAction, Core::Constants::SAVEAS, qmlDesignerMainContext);
    connect(&m_saveAsAction, &QAction::triggered, em, &Core::EditorManager::saveDocumentAs);

    //Export as Image
    command = Core::ActionManager::registerAction(&m_exportAsImageAction, QmlDesigner::Constants::EXPORT_AS_IMAGE, qmlDesignerMainContext);
    command->setAttribute(Core::Command::CA_Hide);
    connect(&m_exportAsImageAction, &QAction::triggered, [] {
        QmlDesignerPlugin::instance()->viewManager().exportAsImage();
    });
    fileMenu->addAction(command, Core::Constants::G_FILE_SAVE);

    //Close Editor
    Core::ActionManager::registerAction(&m_closeCurrentEditorAction, Core::Constants::CLOSE, qmlDesignerMainContext);
    connect(&m_closeCurrentEditorAction, &QAction::triggered, em, &Core::EditorManager::slotCloseCurrentEditorOrDocument);

    DesignerActionManager &designerActionManager = QmlDesignerPlugin::instance()->viewManager().designerActionManager();

    //Close All
    Core::ActionManager::registerAction(&m_closeAllEditorsAction, Core::Constants::CLOSEALL, qmlDesignerMainContext);
    connect(&m_closeAllEditorsAction, &QAction::triggered, em,  &Core::EditorManager::closeAllEditors);

    //Close All Others Action
    Core::ActionManager::registerAction(&m_closeOtherEditorsAction, Core::Constants::CLOSEOTHERS, qmlDesignerMainContext);
    connect(&m_closeOtherEditorsAction, &QAction::triggered, em, [] {
        Core::EditorManager::closeOtherDocuments();
    });

    // Undo / Redo
    command = Core::ActionManager::registerAction(&m_undoAction, Core::Constants::UNDO, qmlDesignerMainContext);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 310, Utils::Icons::UNDO_TOOLBAR.icon());
    command = Core::ActionManager::registerAction(&m_redoAction, Core::Constants::REDO, qmlDesignerMainContext);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 300, Utils::Icons::REDO_TOOLBAR.icon());

    //Edit Menu

    m_deleteAction.setIcon(QIcon::fromTheme(QLatin1String("edit-cut"), Utils::Icons::EDIT_CLEAR_TOOLBAR.icon()));

    command = Core::ActionManager::registerAction(&m_deleteAction, QmlDesigner::Constants::C_DELETE, qmlDesignerMainContext);
    command->setDefaultKeySequence(QKeySequence::Delete);
    command->setAttribute(Core::Command::CA_Hide); // don't show delete in other modes
    if (!Utils::HostOsInfo::isMacHost())
        editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 280);

    Core::ActionManager::registerAction(&m_cutAction, Core::Constants::CUT, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_cutAction, Core::Constants::CUT, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Cut);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 260, Utils::Icons::CUT_TOOLBAR.icon());

    Core::ActionManager::registerAction(&m_copyAction, Core::Constants::COPY, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_copyAction,  Core::Constants::COPY, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Copy);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 250, Utils::Icons::COPY_TOOLBAR.icon());

    Core::ActionManager::registerAction(&m_pasteAction,  Core::Constants::PASTE, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_pasteAction,  Core::Constants::PASTE, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Paste);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 240, Utils::Icons::PASTE_TOOLBAR.icon());

    Core::ActionManager::registerAction(&m_selectAllAction, Core::Constants::SELECTALL, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_selectAllAction, Core::Constants::SELECTALL, qmlDesignerNavigatorContext);

    command->setDefaultKeySequence(QKeySequence::SelectAll);
    editMenu->addAction(command, Core::Constants::G_EDIT_SELECTALL);

    Core::ActionContainer *viewsMenu = Core::ActionManager::actionContainer(Core::Constants::M_WINDOW_VIEWS);

    Core::ActionManager::registerAction(&m_toggleLeftSidebarAction, Core::Constants::TOGGLE_LEFT_SIDEBAR, qmlDesignerMainContext);
    Core::ActionManager::registerAction(&m_toggleRightSidebarAction, Core::Constants::TOGGLE_RIGHT_SIDEBAR, qmlDesignerMainContext);

    command = Core::ActionManager::registerAction(&m_collapseExpandStatesAction,  Constants::TOGGLE_STATES_EDITOR, qmlDesignerMainContext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setDefaultKeySequence(QKeySequence("Ctrl+Alt+s"));
    viewsMenu->addAction(command);

    command = Core::ActionManager::registerAction(&m_restoreDefaultViewAction,  Constants::RESTORE_DEFAULT_VIEW, qmlDesignerMainContext);
    command->setAttribute(Core::Command::CA_Hide);
    viewsMenu->addAction(command);

    command = Core::ActionManager::registerAction(&m_switchTextFormAction, QmlDesigner::Constants::SWITCH_TEXT_DESIGN, qmlDesignerMainContext);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_F4));

    /* Registering disabled action for Escape, because Qt Quick does not support shortcut overrides. */
    command = Core::ActionManager::registerAction(&m_escapeAction, Core::Constants::S_RETURNTOEDITOR, qmlDesignerMainContext);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_Escape));
    m_escapeAction.setEnabled(false);

    connect(designerActionManager.view(), &DesignerActionManagerView::selectionChanged, this, [this](bool itemsSelected, bool rootItemIsSelected) {
        m_deleteAction.setEnabled(itemsSelected && !rootItemIsSelected);
        m_cutAction.setEnabled(itemsSelected && !rootItemIsSelected);
        m_copyAction.setEnabled(itemsSelected);
    });

    connect(Core::ICore::instance(), &Core::ICore::contextChanged, this, [&designerActionManager, this](const Core::Context &context){
        if (!context.contains(Constants::C_QMLFORMEDITOR) && !context.contains(Constants::C_QMLNAVIGATOR)) {
            m_deleteAction.setEnabled(false);
            m_cutAction.setEnabled(false);
            m_copyAction.setEnabled(false);
            m_pasteAction.setEnabled(false);
        } else {
            designerActionManager.view()->emitSelectionChanged();

        }
    });

    updateClipboard(&m_pasteAction);
    connect(QApplication::clipboard(), &QClipboard::QClipboard::changed, this, [this]() {
        updateClipboard(&m_pasteAction);
    });
}

void ShortCutManager::updateActions(Core::IEditor* currentEditor)
{
    int openedCount = Core::DocumentModel::entryCount();

    Core::IDocument *document = 0;
    if (currentEditor)
        document = currentEditor->document();
    m_saveAction.setEnabled(document && document->isModified());
    m_saveAsAction.setEnabled(document && document->isSaveAsAllowed());
    m_revertToSavedAction.setEnabled(document
                                      && !document->filePath().isEmpty()
                                      && document->isModified());

    QString quotedName;
    if (currentEditor && document)
        quotedName = '"' + document->displayName() + '"';

    m_saveAsAction.setText(tr("Save %1 As...").arg(quotedName));
    m_saveAction.setText(tr("&Save %1").arg(quotedName));
    m_revertToSavedAction.setText(tr("Revert %1 to Saved").arg(quotedName));

    m_closeCurrentEditorAction.setEnabled(currentEditor != 0);
    m_closeCurrentEditorAction.setText(tr("Close %1").arg(quotedName));
    m_closeAllEditorsAction.setEnabled(openedCount > 0);
    m_closeOtherEditorsAction.setEnabled(openedCount > 1);
    m_closeOtherEditorsAction.setText((openedCount > 1 ? tr("Close All Except %1").arg(quotedName) : tr("Close Others")));
}

void ShortCutManager::undo()
{
    if (currentDesignDocument())
        currentDesignDocument()->undo();
}

void ShortCutManager::redo()
{
    if (currentDesignDocument())
        currentDesignDocument()->redo();
}

void ShortCutManager::deleteSelected()
{
    if (currentDesignDocument())
        currentDesignDocument()->deleteSelected();
}

void ShortCutManager::cutSelected()
{
    if (currentDesignDocument())
        currentDesignDocument()->cutSelected();
}

void ShortCutManager::copySelected()
{
    if (currentDesignDocument())
        currentDesignDocument()->copySelected();
}

void ShortCutManager::paste()
{
    if (currentDesignDocument())
        currentDesignDocument()->paste();
}

void ShortCutManager::selectAll()
{
    if (currentDesignDocument())
        currentDesignDocument()->selectAll();
}

void ShortCutManager::toggleLeftSidebar()
{
    QmlDesignerPlugin::instance()->mainWidget()->toggleLeftSidebar();
}

void ShortCutManager::toggleRightSidebar()
{
     QmlDesignerPlugin::instance()->mainWidget()->toggleRightSidebar();
}

void ShortCutManager::connectUndoActions(DesignDocument *designDocument)
{
    if (designDocument) {
        connect(designDocument, &DesignDocument::undoAvailable, this, &ShortCutManager::undoAvailable);
        connect(designDocument, &DesignDocument::redoAvailable, this, &ShortCutManager::redoAvailable);
    }
}

void ShortCutManager::disconnectUndoActions(DesignDocument *designDocument)
{
    if (currentDesignDocument()) {
        disconnect(designDocument, &DesignDocument::undoAvailable, this, &ShortCutManager::undoAvailable);
        disconnect(designDocument, &DesignDocument::redoAvailable, this, &ShortCutManager::redoAvailable);
    }
}

void ShortCutManager::updateUndoActions(DesignDocument *designDocument)
{
    if (designDocument) {
        m_undoAction.setEnabled(designDocument->isUndoAvailable());
        m_redoAction.setEnabled(designDocument->isRedoAvailable());
    } else {
        m_undoAction.setEnabled(false);
        m_redoAction.setEnabled(false);
    }
}

DesignDocument *ShortCutManager::currentDesignDocument() const
{
    return QmlDesignerPlugin::instance()->currentDesignDocument();
}

void ShortCutManager::undoAvailable(bool isAvailable)
{
    DesignDocument *documentController = qobject_cast<DesignDocument*>(sender());
    if (currentDesignDocument() &&
        currentDesignDocument() == documentController) {
        m_undoAction.setEnabled(isAvailable);
    }
}

void ShortCutManager::redoAvailable(bool isAvailable)
{
    DesignDocument *documentController = qobject_cast<DesignDocument*>(sender());
    if (currentDesignDocument() &&
        currentDesignDocument() == documentController) {
        m_redoAction.setEnabled(isAvailable);
    }
}

void ShortCutManager::goIntoComponent()
{
    if (currentDesignDocument()
            && currentDesignDocument()->currentModel()
            && currentDesignDocument()->rewriterView()
            && currentDesignDocument()->rewriterView()->hasSingleSelectedModelNode()) {
        DocumentManager::goIntoComponent(currentDesignDocument()->rewriterView()->singleSelectedModelNode());
    }
}

} // namespace QmlDesigner
