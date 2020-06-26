/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "actionhandler.h"

#include "modeleditor_constants.h"
#include "modeleditor.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <utils/icon.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QShortcut>
#include <QMenu>

namespace ModelEditor {
namespace Internal {

class ActionHandler::ActionHandlerPrivate {
public:
    Core::Context context{Constants::MODEL_EDITOR_ID};
    QAction *undoAction = nullptr;
    QAction *redoAction = nullptr;
    QAction *cutAction = nullptr;
    QAction *copyAction = nullptr;
    QAction *pasteAction = nullptr;
    QAction *removeAction = nullptr;
    QAction *deleteAction = nullptr;
    QAction *selectAllAction = nullptr;
    QAction *openParentDiagramAction = nullptr;
    QAction *synchronizeBrowserAction = nullptr;
    QAction *exportDiagramAction = nullptr;
    QAction *exportSelectedElementsAction = nullptr;
};

ActionHandler::ActionHandler()
    : d(new ActionHandlerPrivate)
{
}

ActionHandler::~ActionHandler()
{
    delete d;
}

QAction *ActionHandler::undoAction() const
{
    return d->undoAction;
}

QAction *ActionHandler::redoAction() const
{
    return d->redoAction;
}

QAction *ActionHandler::cutAction() const
{
    return d->cutAction;
}

QAction *ActionHandler::copyAction() const
{
    return d->copyAction;
}

QAction *ActionHandler::pasteAction() const
{
    return d->pasteAction;
}

QAction *ActionHandler::removeAction() const
{
    return d->removeAction;
}

QAction *ActionHandler::deleteAction() const
{
    return d->deleteAction;
}

QAction *ActionHandler::selectAllAction() const
{
    return d->selectAllAction;
}

QAction *ActionHandler::openParentDiagramAction() const
{
    return d->openParentDiagramAction;
}

QAction *ActionHandler::synchronizeBrowserAction() const
{
    return d->synchronizeBrowserAction;
}

QAction *ActionHandler::exportDiagramAction() const
{
    return d->exportDiagramAction;
}

QAction *ActionHandler::exportSelectedElementsAction() const
{
    return d->exportSelectedElementsAction;
}

void ActionHandler::createActions()
{
    Core::ActionContainer *medit = Core::ActionManager::actionContainer(Core::Constants::M_EDIT);
    Core::ActionContainer *mfile = Core::ActionManager::actionContainer(Core::Constants::M_FILE);

    d->undoAction = registerCommand(Core::Constants::UNDO, &ModelEditor::undo, d->context)->action();
    d->redoAction = registerCommand(Core::Constants::REDO, &ModelEditor::redo, d->context)->action();
    d->cutAction = registerCommand(Core::Constants::CUT, &ModelEditor::cut, d->context)->action();
    d->copyAction = registerCommand(Core::Constants::COPY, &ModelEditor::copy, d->context)->action();
    d->pasteAction = registerCommand(Core::Constants::PASTE, &ModelEditor::paste, d->context)->action();
    Core::Command *removeCommand = registerCommand(
                Constants::REMOVE_SELECTED_ELEMENTS, &ModelEditor::removeSelectedElements, d->context,
                tr("&Remove"), QKeySequence::Delete);
    medit->addAction(removeCommand, Core::Constants::G_EDIT_COPYPASTE);
    d->removeAction = removeCommand->action();
    Core::Command *deleteCommand = registerCommand(
                Constants::DELETE_SELECTED_ELEMENTS, &ModelEditor::deleteSelectedElements, d->context,
                tr("&Delete"), QKeySequence("Ctrl+D"));
    medit->addAction(deleteCommand, Core::Constants::G_EDIT_COPYPASTE);
    d->deleteAction = deleteCommand->action();
    d->selectAllAction = registerCommand(Core::Constants::SELECTALL, &ModelEditor::selectAll, d->context)->action();

    Core::Command *exportDiagramCommand = registerCommand(
                Constants::EXPORT_DIAGRAM, &ModelEditor::exportDiagram, d->context,
                tr("Export Diagram..."));
    exportDiagramCommand->setAttribute(Core::Command::CA_Hide);
    mfile->addAction(exportDiagramCommand, Core::Constants::G_FILE_EXPORT);
    d->exportDiagramAction = exportDiagramCommand->action();

    Core::Command *exportSelectedElementsCommand = registerCommand(
                Constants::EXPORT_SELECTED_ELEMENTS, &ModelEditor::exportSelectedElements, d->context,
                tr("Export Selected Elements..."));
    exportSelectedElementsCommand->setAttribute(Core::Command::CA_Hide);
    mfile->addAction(exportSelectedElementsCommand, Core::Constants::G_FILE_EXPORT);
    d->exportSelectedElementsAction = exportSelectedElementsCommand->action();

    registerCommand(Core::Constants::ZOOM_IN, &ModelEditor::zoomIn, d->context);
    registerCommand(Core::Constants::ZOOM_OUT, &ModelEditor::zoomOut, d->context);
    registerCommand(Core::Constants::ZOOM_RESET, &ModelEditor::resetZoom, d->context);

    d->openParentDiagramAction = registerCommand(
                Constants::OPEN_PARENT_DIAGRAM, &ModelEditor::openParentDiagram, Core::Context(),
                tr("Open Parent Diagram"), QKeySequence("Ctrl+Shift+P"),
                QIcon(":/modeleditor/up.png"))->action();
    registerCommand(Constants::ACTION_ADD_PACKAGE, nullptr, Core::Context(), tr("Add Package"),
                    QKeySequence(), QIcon(":/modelinglib/48x48/package.png"));
    registerCommand(Constants::ACTION_ADD_COMPONENT, nullptr, Core::Context(), tr("Add Component"),
                    QKeySequence(), QIcon(":/modelinglib/48x48/component.png"));
    registerCommand(Constants::ACTION_ADD_CLASS, nullptr, Core::Context(), tr("Add Class"),
                    QKeySequence(), QIcon(":/modelinglib/48x48/class.png"));
    registerCommand(Constants::ACTION_ADD_CANVAS_DIAGRAM, nullptr, Core::Context(), tr("Add Canvas Diagram"),
                    QKeySequence(), QIcon(":/modelinglib/48x48/canvas-diagram.png"));
    d->synchronizeBrowserAction = registerCommand(
                Constants::ACTION_SYNC_BROWSER, nullptr, Core::Context(),
                tr("Synchronize Browser and Diagram") + "<br><i><small>"
                + tr("Press && Hold for Options") + "</small></i>", QKeySequence(),
                Utils::Icons::LINK_TOOLBAR.icon())->action();
    d->synchronizeBrowserAction->setCheckable(true);

    auto editPropertiesAction = new QAction(tr("Edit Element Properties"),
                                            Core::ICore::dialogParent());
    Core::Command *editPropertiesCommand = Core::ActionManager::registerAction(
                editPropertiesAction, Constants::SHORTCUT_MODEL_EDITOR_EDIT_PROPERTIES, d->context);
    editPropertiesCommand->setDefaultKeySequence(QKeySequence(tr("Shift+Return")));
    connect(editPropertiesAction, &QAction::triggered, this, &ActionHandler::onEditProperties);

    auto editItemAction = new QAction(tr("Edit Item on Diagram"), Core::ICore::dialogParent());
    Core::Command *editItemCommand = Core::ActionManager::registerAction(
                editItemAction, Constants::SHORTCUT_MODEL_EDITOR_EDIT_ITEM, d->context);
    editItemCommand->setDefaultKeySequence(QKeySequence(tr("Return")));
    connect(editItemAction, &QAction::triggered, this, &ActionHandler::onEditItem);
}

void ActionHandler::onEditProperties()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->editProperties();
}

void ActionHandler::onEditItem()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->editSelectedItem();
}

std::function<void()> invokeOnCurrentModelEditor(void (ModelEditor::*function)())
{
    return [function] {
        auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
        if (editor)
            (editor->*function)();
    };
}

Core::Command *ActionHandler::registerCommand(const Utils::Id &id, void (ModelEditor::*function)(),
                                              const Core::Context &context, const QString &title,
                                              const QKeySequence &keySequence, const QIcon &icon)
{
    auto action = new QAction(title, this);
    if (!icon.isNull())
        action->setIcon(icon);
    Core::Command *command = Core::ActionManager::registerAction(action, id, context, /*scriptable=*/true);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(keySequence);
    if (function)
        connect(action, &QAction::triggered, this, invokeOnCurrentModelEditor(function));
    return command;
}

} // namespace Internal
} // namespace ModelEditor
