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

#include "shortcutmanager.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <utils/hostosinfo.h>

#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "designmodewidget.h"


namespace QmlDesigner {

ShortCutManager::ShortCutManager()
    : QObject(),
    m_revertToSavedAction(0),
    m_saveAction(0),
    m_saveAsAction(0),
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
    m_hideSidebarsAction(tr("Toggle Sidebars"), 0),
    m_restoreDefaultViewAction(tr("&Restore Default View"), 0),
    m_toggleLeftSidebarAction(tr("Toggle &Left Sidebar"), 0),
    m_toggleRightSidebarAction(tr("Toggle &Right Sidebar"), 0),
    m_goIntoComponentAction (tr("&Go into Component"), 0)
{

}

void ShortCutManager::registerActions(const Core::Context &qmlDesignerMainContext,
                                      const Core::Context &qmlDesignerFormEditorContext,
                                      const Core::Context &qmlDesignerNavigatorContext)
{
    Core::ActionContainer *editMenu = Core::ActionManager::actionContainer(Core::Constants::M_EDIT);

    connect(&m_undoAction, SIGNAL(triggered()), this, SLOT(undo()));

    connect(&m_redoAction, SIGNAL(triggered()), this, SLOT(redo()));

    connect(&m_deleteAction, SIGNAL(triggered()), this, SLOT(deleteSelected()));

    connect(&m_cutAction, SIGNAL(triggered()), this, SLOT(cutSelected()));

    connect(&m_copyAction, SIGNAL(triggered()), this, SLOT(copySelected()));

    connect(&m_pasteAction, SIGNAL(triggered()), this, SLOT(paste()));

    connect(&m_selectAllAction, SIGNAL(triggered()), this, SLOT(selectAll()));

    connect(&m_hideSidebarsAction, SIGNAL(triggered()), this, SLOT(toggleSidebars()));

    connect(&m_restoreDefaultViewAction,
            SIGNAL(triggered()),
            QmlDesignerPlugin::instance()->mainWidget(),
            SLOT(restoreDefaultView()));

    connect(&m_goIntoComponentAction, SIGNAL(triggered()), SLOT(goIntoComponent()));

    connect(&m_toggleLeftSidebarAction,
            SIGNAL(triggered()),
            QmlDesignerPlugin::instance()->mainWidget(),
            SLOT(toggleLeftSidebar()));

    connect(&m_toggleRightSidebarAction,
            SIGNAL(triggered()),
            QmlDesignerPlugin::instance()->mainWidget(),
            SLOT(toggleRightSidebar()));

    // Revert to saved
    QObject *em = Core::EditorManager::instance();
    Core::ActionManager::registerAction(&m_revertToSavedAction,Core::Constants::REVERTTOSAVED, qmlDesignerMainContext);
    connect(&m_revertToSavedAction, SIGNAL(triggered()), em, SLOT(revertToSaved()));

    //Save
    Core::ActionManager::registerAction(&m_saveAction, Core::Constants::SAVE, qmlDesignerMainContext);
    connect(&m_saveAction, SIGNAL(triggered()), em, SLOT(saveDocument()));

    //Save As
    Core::ActionManager::registerAction(&m_saveAsAction, Core::Constants::SAVEAS, qmlDesignerMainContext);
    connect(&m_saveAsAction, SIGNAL(triggered()), em, SLOT(saveDocumentAs()));

    //Close Editor
    Core::ActionManager::registerAction(&m_closeCurrentEditorAction, Core::Constants::CLOSE, qmlDesignerMainContext);
    connect(&m_closeCurrentEditorAction, SIGNAL(triggered()), em, SLOT(slotCloseCurrentEditorOrDocument()));

    //Close All
    Core::ActionManager::registerAction(&m_closeAllEditorsAction, Core::Constants::CLOSEALL, qmlDesignerMainContext);
    connect(&m_closeAllEditorsAction, SIGNAL(triggered()), em, SLOT(closeAllEditors()));

    //Close All Others Action
    Core::ActionManager::registerAction(&m_closeOtherEditorsAction, Core::Constants::CLOSEOTHERS, qmlDesignerMainContext);
    connect(&m_closeOtherEditorsAction, SIGNAL(triggered()), em, SLOT(closeOtherDocuments()));

    // Undo / Redo
    Core::ActionManager::registerAction(&m_undoAction, Core::Constants::UNDO, qmlDesignerMainContext);
    Core::ActionManager::registerAction(&m_redoAction, Core::Constants::REDO, qmlDesignerMainContext);

    Core::Command *command;

    //GoIntoComponent
    command = Core::ActionManager::registerAction(&m_goIntoComponentAction,
                                                  Constants::GO_INTO_COMPONENT, qmlDesignerMainContext);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_F2));

    //Edit Menu

    Core::ActionManager::registerAction(&m_deleteAction, QmlDesigner::Constants::C_BACKSPACE, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_deleteAction, QmlDesigner::Constants::C_BACKSPACE, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_Backspace));
    command->setAttribute(Core::Command::CA_Hide); // don't show delete in other modes
    if (Utils::HostOsInfo::isMacHost())
        editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    Core::ActionManager::registerAction(&m_deleteAction, QmlDesigner::Constants::C_DELETE, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_deleteAction, QmlDesigner::Constants::C_DELETE, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Delete);
    command->setAttribute(Core::Command::CA_Hide); // don't show delete in other modes
    if (!Utils::HostOsInfo::isMacHost())
        editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    Core::ActionManager::registerAction(&m_cutAction, Core::Constants::CUT, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_cutAction, Core::Constants::CUT, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Cut);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    Core::ActionManager::registerAction(&m_copyAction, Core::Constants::COPY, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_copyAction,  Core::Constants::COPY, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Copy);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    Core::ActionManager::registerAction(&m_pasteAction,  Core::Constants::PASTE, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_pasteAction,  Core::Constants::PASTE, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Paste);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    Core::ActionManager::registerAction(&m_selectAllAction, Core::Constants::SELECTALL, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_selectAllAction, Core::Constants::SELECTALL, qmlDesignerNavigatorContext);

    command->setDefaultKeySequence(QKeySequence::SelectAll);
    editMenu->addAction(command, Core::Constants::G_EDIT_SELECTALL);

    Core::ActionContainer *viewsMenu = Core::ActionManager::actionContainer(Core::Constants::M_WINDOW_VIEWS);

    command = Core::ActionManager::registerAction(&m_toggleLeftSidebarAction,  Constants::TOGGLE_LEFT_SIDEBAR, qmlDesignerMainContext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setDefaultKeySequence(QKeySequence("Ctrl+Alt+0"));
    viewsMenu->addAction(command);

    command = Core::ActionManager::registerAction(&m_toggleRightSidebarAction, Constants::TOGGLE_RIGHT_SIDEBAR, qmlDesignerMainContext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setDefaultKeySequence(QKeySequence("Ctrl+Alt+Shift+0"));
    viewsMenu->addAction(command);

    command = Core::ActionManager::registerAction(&m_restoreDefaultViewAction,  Constants::RESTORE_DEFAULT_VIEW, qmlDesignerMainContext);
    command->setAttribute(Core::Command::CA_Hide);
    viewsMenu->addAction(command);

    Core::ActionManager::registerAction(&m_hideSidebarsAction, Core::Constants::TOGGLE_SIDEBAR, qmlDesignerMainContext);
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

void ShortCutManager::toggleSidebars()
{
    QmlDesignerPlugin::instance()->mainWidget()->toggleSidebars();
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
        connect(designDocument, SIGNAL(undoAvailable(bool)), this, SLOT(undoAvailable(bool)));
        connect(designDocument, SIGNAL(redoAvailable(bool)), this, SLOT(redoAvailable(bool)));
    }
}

void ShortCutManager::disconnectUndoActions(DesignDocument *designDocument)
{
    if (currentDesignDocument()) {
        disconnect(designDocument, SIGNAL(undoAvailable(bool)), this, SLOT(undoAvailable(bool)));
        disconnect(designDocument, SIGNAL(redoAvailable(bool)), this, SLOT(redoAvailable(bool)));
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
