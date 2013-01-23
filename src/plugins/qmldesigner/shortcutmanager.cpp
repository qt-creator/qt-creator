#include "shortcutmanager.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/coreconstants.h>
#include <utils/hostosinfo.h>

#include "qmldesignerconstants.h"
#include "designdocument.h"
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
    m_hideSidebarsAction(tr("Toggle Full Screen"), 0),
    m_restoreDefaultViewAction(tr("&Restore Default View"), 0),
    m_toggleLeftSidebarAction(tr("Toggle &Left Sidebar"), 0),
    m_toggleRightSidebarAction(tr("Toggle &Right Sidebar"), 0),
    m_goIntoComponentAction (tr("&Go into Component"), 0)
{

}

void ShortCutManager::registerActions()
{
    Core::ActionContainer *editMenu = Core::ActionManager::actionContainer(Core::Constants::M_EDIT);

    Core::Context qmlDesignerMainContext(Constants::C_QMLDESIGNER);
    Core::Context qmlDesignerFormEditorContext(Constants::C_QMLFORMEDITOR);
    Core::Context qmlDesignerNavigatorContext(Constants::C_QMLNAVIGATOR);

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
    Core::ActionManager::registerAction(&m_revertToSavedAction,Core::Constants::REVERTTOSAVED, qmlDesignerMainContext);
    connect(&m_revertToSavedAction, SIGNAL(triggered()), Core::ICore::editorManager(), SLOT(revertToSaved()));

    //Save
    Core::ActionManager::registerAction(&m_saveAction, Core::Constants::SAVE, qmlDesignerMainContext);
    connect(&m_saveAction, SIGNAL(triggered()), Core::ICore::editorManager(), SLOT(saveDocument()));

    //Save As
    Core::ActionManager::registerAction(&m_saveAsAction, Core::Constants::SAVEAS, qmlDesignerMainContext);
    connect(&m_saveAsAction, SIGNAL(triggered()), Core::ICore::editorManager(), SLOT(saveDocumentAs()));

    //Close Editor
    Core::ActionManager::registerAction(&m_closeCurrentEditorAction, Core::Constants::CLOSE, qmlDesignerMainContext);
    connect(&m_closeCurrentEditorAction, SIGNAL(triggered()), Core::ICore::editorManager(), SLOT(closeEditor()));

    //Close All
    Core::ActionManager::registerAction(&m_closeAllEditorsAction, Core::Constants::CLOSEALL, qmlDesignerMainContext);
    connect(&m_closeAllEditorsAction, SIGNAL(triggered()), Core::ICore::editorManager(), SLOT(closeAllEditors()));

    //Close All Others Action
    Core::ActionManager::registerAction(&m_closeOtherEditorsAction, Core::Constants::CLOSEOTHERS, qmlDesignerMainContext);
    connect(&m_closeOtherEditorsAction, SIGNAL(triggered()), Core::ICore::editorManager(), SLOT(closeOtherEditors()));

    // Undo / Redo
    Core::ActionManager::registerAction(&m_undoAction, Core::Constants::UNDO, qmlDesignerMainContext);
    Core::ActionManager::registerAction(&m_redoAction, Core::Constants::REDO, qmlDesignerMainContext);

    Core::Command *command;

    //GoIntoComponent
    command = Core::ActionManager::registerAction(&m_goIntoComponentAction,
                                                  Constants::GO_INTO_COMPONENT, qmlDesignerMainContext);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_F2));

    //Edit Menu

    command = Core::ActionManager::registerAction(&m_deleteAction, QmlDesigner::Constants::DELETE, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_deleteAction, QmlDesigner::Constants::DELETE, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Delete);
    command->setAttribute(Core::Command::CA_Hide); // don't show delete in other modes
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = Core::ActionManager::registerAction(&m_cutAction, Core::Constants::CUT, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_cutAction, Core::Constants::CUT, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Cut);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = Core::ActionManager::registerAction(&m_copyAction, Core::Constants::COPY, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_copyAction,  Core::Constants::COPY, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Copy);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = Core::ActionManager::registerAction(&m_pasteAction,  Core::Constants::PASTE, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_pasteAction,  Core::Constants::PASTE, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Paste);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = Core::ActionManager::registerAction(&m_selectAllAction, Core::Constants::SELECTALL, qmlDesignerFormEditorContext);
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

    command = Core::ActionManager::registerAction(&m_hideSidebarsAction, Core::Constants::TOGGLE_SIDEBAR, qmlDesignerMainContext);

    if (Utils::HostOsInfo::isMacHost()) {
        // add second shortcut to trigger delete
        QAction *deleteAction = new QAction(this);
        deleteAction->setShortcut(QKeySequence(QLatin1String("Backspace")));
        connect(deleteAction,
                SIGNAL(triggered()),
                &m_deleteAction,
                SIGNAL(triggered()));
    }
}

void ShortCutManager::updateActions(Core::IEditor* editor)
{
    Core::IEditor *currentEditor = editor;
    int openedCount = Core::ICore::editorManager()->openedEditors().count()
                      + Core::ICore::editorManager()->openedEditorsModel()->restoredEditors().count();

    QString fileName;
    if (currentEditor) {
        if (!currentEditor->document()->fileName().isEmpty()) {
            QFileInfo fileInfo(currentEditor->document()->fileName());
            fileName = fileInfo.fileName();
        } else {
            fileName = currentEditor->displayName();
        }
    }

    m_saveAction.setEnabled(currentEditor != 0 && currentEditor->document()->isModified());
    m_saveAsAction.setEnabled(currentEditor != 0 && currentEditor->document()->isSaveAsAllowed());
    m_revertToSavedAction.setEnabled(currentEditor != 0
                                      && !currentEditor->document()->fileName().isEmpty()
                                      && currentEditor->document()->isModified());

    QString quotedName;
    if (!fileName.isEmpty())
        quotedName = '"' + fileName + '"';

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
    if (currentDesignDocument())
        currentDesignDocument()->goIntoComponent();
}

} // namespace QmlDesigner
