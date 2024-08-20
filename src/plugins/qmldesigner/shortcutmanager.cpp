// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "shortcutmanager.h"

#include <designersettings.h>

#include <toolbarbackend.h>

#include <viewmanager.h>
#include <designeractionmanagerview.h>
#include <componentcore_constants.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/modemanager.h>
#include <qmljseditor/qmljseditorconstants.h>

#include <qmlprojectmanager/qmlprojectconstants.h>

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/proxyaction.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <qmljs/qmljsreformatter.h>

#include "modelnodecontextmenu_helper.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"

#include <QApplication>
#include <QClipboard>
#include <QMainWindow>
#include <QStandardPaths>

namespace QmlDesigner {

constexpr char deleteActionId[] = "QmlDesigner.Delete";
constexpr char duplicateActionId[] = "QmlDesigner.Duplicate";

static DesignDocument *currentDesignDocument()
{
    return QmlDesignerPlugin::instance()->currentDesignDocument();
}

ShortCutManager::ShortCutManager()
    : QObject()
    , m_exportAsImageAction(tr("Export as &Image..."))
    , m_takeScreenshotAction(tr("Take Screenshot"))
    , m_undoAction(tr("&Undo"))
    , m_redoAction(tr("&Redo"))
    , m_deleteAction(tr("Delete"))
    , m_cutAction(tr("Cu&t"))
    , m_copyAction(tr("&Copy"))
    , m_pasteAction(tr("&Paste"))
    , m_duplicateAction(tr("&Duplicate"))
    , m_selectAllAction(tr("Select &All"))
    , m_escapeAction(this)
{}

void ShortCutManager::registerActions(const Core::Context &qmlDesignerMainContext,
                                      const Core::Context &qmlDesignerFormEditorContext,
                                      const Core::Context &qmlDesignerEditor3DContext,
                                      const Core::Context &qmlDesignerNavigatorContext)
{
    Core::ActionContainer *editMenu = Core::ActionManager::actionContainer(Core::Constants::M_EDIT);

    connect(&m_undoAction, &QAction::triggered, this, &ShortCutManager::undo);

    connect(&m_redoAction, &QAction::triggered, this, &ShortCutManager::redo);

    connect(&m_deleteAction, &QAction::triggered, this, &ShortCutManager::deleteSelected);

    connect(&m_duplicateAction, &QAction::triggered, this, &ShortCutManager::duplicateSelected);

    connect(&m_cutAction, &QAction::triggered, this, &ShortCutManager::cutSelected);

    connect(&m_copyAction, &QAction::triggered, this, &ShortCutManager::copySelected);

    connect(&m_pasteAction, &QAction::triggered, this, &ShortCutManager::paste);

    connect(&m_selectAllAction,&QAction::triggered, this, &ShortCutManager::selectAll);

    // Revert to saved
    Core::EditorManager *em = Core::EditorManager::instance();
    Core::ActionManager::registerAction(&m_revertToSavedAction,Core::Constants::REVERTTOSAVED, qmlDesignerMainContext);
    connect(&m_revertToSavedAction, &QAction::triggered, em, &Core::EditorManager::revertToSaved);

    //Save
    Core::ActionManager::registerAction(&m_saveAction, Core::Constants::SAVE, qmlDesignerMainContext);
    connect(&m_saveAction, &QAction::triggered, em, [] {
         QmlDesignerPlugin::instance()->viewManager().reformatFileUsingTextEditorView();
         Core::EditorManager::saveDocument();
    });

    Core::Command *command = nullptr;

    //Save As
    Core::ActionManager::registerAction(&m_saveAsAction, Core::Constants::SAVEAS, qmlDesignerMainContext);
    connect(&m_saveAsAction, &QAction::triggered, em, &Core::EditorManager::saveDocumentAs);

    //Export as Image
    static constexpr char exportAsImageActionId[] = "QmlDesigner.ExportAsImage";
    command = Core::ActionManager::registerAction(&m_exportAsImageAction,
                                                  exportAsImageActionId,
                                                  qmlDesignerMainContext);
    command->setAttribute(Core::Command::CA_Hide);
    connect(&m_exportAsImageAction, &QAction::triggered, [] {
        QmlDesignerPlugin::instance()->viewManager().exportAsImage();
    });

    // Edit Global Annotations
    QAction *action = new QAction(tr("Edit Global Annotations..."), this);
    command = Core::ActionManager::registerAction(action, "Edit.Annotations", qmlDesignerMainContext);
    Core::ActionManager::actionContainer(Core::Constants::M_EDIT)
        ->addAction(command, Core::Constants::G_EDIT_OTHER);
    connect(action, &QAction::triggered, this, [] { ToolBarBackend::launchGlobalAnnotations(); });
    connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeChanged, this, [action] {
        action->setEnabled(Core::ModeManager::currentModeId() == Core::Constants::MODE_DESIGN);
    });
    action->setEnabled(false);

    static constexpr char takeScreenShotActionId[] = "QmlDesigner.TakeScreenshot";
    command = Core::ActionManager::registerAction(&m_takeScreenshotAction, takeScreenShotActionId);
    connect(&m_takeScreenshotAction, &QAction::triggered, [] {
        const auto folder = Utils::FilePath::fromString(
                                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                .pathAppended("QtDesignStudio/screenshots/");
        folder.createDir();

        const auto file = folder.pathAppended(QDateTime::currentDateTime().toString("dddd-hh-mm-ss")
                                              + ".png");

        QPixmap pixmap = Core::ICore::mainWindow()->grab();

        const bool b = pixmap.save(file.toString(), "PNG");
        qWarning() << "screenshot" << file << b << pixmap;
    });

    Core::ActionContainer *exportMenu = Core::ActionManager::actionContainer(
        QmlProjectManager::Constants::EXPORT_MENU);

    if (exportMenu)
        exportMenu->addAction(command, QmlProjectManager::Constants::G_EXPORT_CONVERT);

    //Close Editor
    Core::ActionManager::registerAction(&m_closeCurrentEditorAction, Core::Constants::CLOSE, qmlDesignerMainContext);
    connect(&m_closeCurrentEditorAction, &QAction::triggered, em, &Core::EditorManager::slotCloseCurrentEditorOrDocument);

    DesignerActionManager &designerActionManager = QmlDesignerPlugin::instance()->viewManager().designerActionManager();

    //Close All
    Core::ActionManager::registerAction(&m_closeAllEditorsAction, Core::Constants::CLOSEALL, qmlDesignerMainContext);
    connect(&m_closeAllEditorsAction, &QAction::triggered, em,  &Core::EditorManager::closeAllDocuments);

    //Close All Others Action
    Core::ActionManager::registerAction(&m_closeOtherEditorsAction, Core::Constants::CLOSEOTHERS, qmlDesignerMainContext);
    connect(&m_closeOtherEditorsAction, &QAction::triggered, em, [] {
        Core::EditorManager::closeOtherDocuments();
    });

    // Undo / Redo
    command = Core::ActionManager::registerAction(&m_undoAction, Core::Constants::UNDO, qmlDesignerMainContext);
    command->setDefaultKeySequence(QKeySequence::Undo);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 1, Utils::Icons::UNDO_TOOLBAR.icon());
    command = Core::ActionManager::registerAction(&m_redoAction, Core::Constants::REDO, qmlDesignerMainContext);
    command->setDefaultKeySequence(QKeySequence::Redo);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 2, Utils::Icons::REDO_TOOLBAR.icon());

    designerActionManager.addDesignerAction(new SeparatorDesignerAction(ComponentCoreConstants::editCategory, 10));
    //Edit Menu

    m_deleteAction.setIcon(QIcon::fromTheme(QLatin1String("edit-cut"), Utils::Icons::EDIT_CLEAR_TOOLBAR.icon()));

    command = Core::ActionManager::registerAction(&m_deleteAction,
                                                  deleteActionId,
                                                  qmlDesignerMainContext);
    command->setDefaultKeySequences({Qt::Key_Backspace, Qt::Key_Delete});

    command->setAttribute(Core::Command::CA_Hide); // don't show delete in other modes
    if (!Utils::HostOsInfo::isMacHost())
        editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 14);

    Core::ActionManager::registerAction(&m_cutAction, Core::Constants::CUT, qmlDesignerFormEditorContext);
    Core::ActionManager::registerAction(&m_cutAction, Core::Constants::CUT, qmlDesignerEditor3DContext);
    command = Core::ActionManager::registerAction(&m_cutAction, Core::Constants::CUT, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Cut);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 13, Utils::Icons::CUT_TOOLBAR.icon());

    Core::ActionManager::registerAction(&m_copyAction, Core::Constants::COPY, qmlDesignerFormEditorContext);
    Core::ActionManager::registerAction(&m_copyAction, Core::Constants::COPY, qmlDesignerEditor3DContext);
    command = Core::ActionManager::registerAction(&m_copyAction,  Core::Constants::COPY, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Copy);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 11, Utils::Icons::COPY_TOOLBAR.icon());

    Core::ActionManager::registerAction(&m_pasteAction,  Core::Constants::PASTE, qmlDesignerFormEditorContext);
    Core::ActionManager::registerAction(&m_pasteAction,  Core::Constants::PASTE, qmlDesignerEditor3DContext);
    command = Core::ActionManager::registerAction(&m_pasteAction,  Core::Constants::PASTE, qmlDesignerNavigatorContext);
    command->setDefaultKeySequence(QKeySequence::Paste);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 12, Utils::Icons::PASTE_TOOLBAR.icon());

    Core::ActionManager::registerAction(&m_duplicateAction,
                                        duplicateActionId,
                                        qmlDesignerFormEditorContext);
    Core::ActionManager::registerAction(&m_duplicateAction,
                                        duplicateActionId,
                                        qmlDesignerEditor3DContext);
    command = Core::ActionManager::registerAction(&m_duplicateAction,
                                                  duplicateActionId,
                                                  qmlDesignerMainContext);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);
    designerActionManager.addCreatorCommand(command, ComponentCoreConstants::editCategory, 15);

    Core::ActionManager::registerAction(&m_selectAllAction, Core::Constants::SELECTALL, qmlDesignerFormEditorContext);
    command = Core::ActionManager::registerAction(&m_selectAllAction, Core::Constants::SELECTALL, qmlDesignerNavigatorContext);

    command->setDefaultKeySequence(QKeySequence::SelectAll);
    editMenu->addAction(command, Core::Constants::G_EDIT_SELECTALL);

    /* Registering disabled action for Escape, because Qt Quick does not support shortcut overrides. */
    command = Core::ActionManager::registerAction(&m_escapeAction, Core::Constants::S_RETURNTOEDITOR, qmlDesignerMainContext);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_Escape));
    m_escapeAction.setEnabled(false);

    connect(designerActionManager.view(), &DesignerActionManagerView::selectionChanged, this, [this](bool itemsSelected, bool rootItemIsSelected) {
        m_deleteAction.setEnabled(itemsSelected && !rootItemIsSelected);
        m_cutAction.setEnabled(itemsSelected && !rootItemIsSelected);
        m_copyAction.setEnabled(itemsSelected);
        m_pasteAction.setEnabled(true);
    });

    connect(Core::ICore::instance(), &Core::ICore::contextChanged, this, [&](const Core::Context &context) {
        isMatBrowserActive = context.contains(Constants::qmlMaterialBrowserContextId);
        isAssetsLibraryActive = context.contains(Constants::qmlAssetsLibraryContextId);

        if (!context.contains(Constants::qmlFormEditorContextId)
            && !context.contains(Constants::qml3DEditorContextId)
            && !context.contains(Constants::qmlNavigatorContextId)) {
            m_deleteAction.setEnabled(isMatBrowserActive || isAssetsLibraryActive);
            m_cutAction.setEnabled(false);
            m_copyAction.setEnabled(false);
            m_pasteAction.setEnabled(false);
        } else {
            designerActionManager.view()->emitSelectionChanged();
        }
    });
}

void ShortCutManager::updateActions(Core::IEditor* currentEditor)
{
    int openedCount = Core::DocumentModel::entryCount();

    Core::IDocument *document = nullptr;
    if (currentEditor)
        document = currentEditor->document();
    m_saveAction.setEnabled(document && document->isModified());
    m_saveAsAction.setEnabled(document && document->isSaveAsAllowed());
    m_revertToSavedAction.setEnabled(document
                                      && !document->filePath().isEmpty()
                                      && document->isModified());

    QString quotedName;
    if (currentEditor && document)
        quotedName = '"' + Utils::quoteAmpersands(document->displayName()) + '"';

    m_saveAsAction.setText(tr("Save %1 As...").arg(quotedName));
    m_saveAction.setText(tr("&Save %1").arg(quotedName));
    m_revertToSavedAction.setText(tr("Revert %1 to Saved").arg(quotedName));

    m_closeCurrentEditorAction.setEnabled(currentEditor != nullptr);
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
    auto &actionManager = QmlDesignerPlugin::instance()->viewManager().designerActionManager();
    if (isMatBrowserActive)
        actionManager.view()->emitCustomNotification("delete_selected_material");
    else if (isAssetsLibraryActive)
        actionManager.view()->emitCustomNotification("delete_selected_assets");
    else if (currentDesignDocument())
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

void ShortCutManager::duplicateSelected()
{
    if (currentDesignDocument())
        currentDesignDocument()->duplicateSelected();
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

void ShortCutManager::connectUndoActions(DesignDocument *designDocument)
{
    if (designDocument) {
        connect(designDocument, &DesignDocument::undoAvailable, this,
                [this, designDocument](bool isAvailable) {
            if (currentDesignDocument() == designDocument)
                m_undoAction.setEnabled(isAvailable);
        });
        connect(designDocument, &DesignDocument::redoAvailable, this,
                [this, designDocument](bool isAvailable) {
            if (currentDesignDocument() == designDocument)
                m_redoAction.setEnabled(isAvailable);
        });
    }
}

void ShortCutManager::disconnectUndoActions(DesignDocument *designDocument)
{
    if (designDocument) {
        disconnect(designDocument, &DesignDocument::undoAvailable, this, nullptr);
        disconnect(designDocument, &DesignDocument::redoAvailable, this, nullptr);
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
