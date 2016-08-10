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
    Core::Context context;
    QAction *undoAction = 0;
    QAction *redoAction = 0;
    QAction *cutAction = 0;
    QAction *copyAction = 0;
    QAction *pasteAction = 0;
    QAction *removeAction = 0;
    QAction *deleteAction = 0;
    QAction *selectAllAction = 0;
    QAction *openParentDiagramAction = 0;
    QAction *synchronizeBrowserAction = 0;
    QAction *exportDiagramAction = 0;
    QAction *zoomInAction = 0;
    QAction *zoomOutAction = 0;
    QAction *resetZoomAction = 0;
};

ActionHandler::ActionHandler(const Core::Context &context, QObject *parent)
    : QObject(parent),
      d(new ActionHandlerPrivate)
{
    d->context = context;
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

QAction *ActionHandler::zoomInAction() const
{
    return d->zoomInAction;
}

QAction *ActionHandler::zoomOutAction() const
{
    return d->zoomOutAction;
}

QAction *ActionHandler::resetZoom() const
{
    return d->resetZoomAction;
}

void ActionHandler::createActions()
{
    Core::ActionContainer *medit = Core::ActionManager::actionContainer(Core::Constants::M_EDIT);

    d->undoAction = registerCommand(Core::Constants::UNDO, [this]() { undo(); }, d->context)->action();
    d->redoAction = registerCommand(Core::Constants::REDO, [this]() { redo(); }, d->context)->action();
    d->cutAction = registerCommand(Core::Constants::CUT, [this]() { cut(); }, d->context)->action();
    d->copyAction = registerCommand(Core::Constants::COPY, [this]() { copy(); }, d->context)->action();
    d->pasteAction = registerCommand(Core::Constants::PASTE, [this]() { paste(); }, d->context)->action();
    Core::Command *removeCommand = registerCommand(
                Constants::REMOVE_SELECTED_ELEMENTS, [this]() { removeSelectedElements(); }, d->context, true,
                tr("&Remove"), QKeySequence::Delete);
    medit->addAction(removeCommand, Core::Constants::G_EDIT_COPYPASTE);
    d->removeAction = removeCommand->action();
    Core::Command *deleteCommand = registerCommand(
                Constants::DELETE_SELECTED_ELEMENTS, [this]() { deleteSelectedElements(); }, d->context, true,
                tr("&Delete"), QKeySequence(QStringLiteral("Ctrl+D")));
    medit->addAction(deleteCommand, Core::Constants::G_EDIT_COPYPASTE);
    d->deleteAction = deleteCommand->action();
    d->selectAllAction = registerCommand(Core::Constants::SELECTALL, [this]() { selectAll(); }, d->context)->action();

    Core::ActionContainer *menuModelEditor = Core::ActionManager::createMenu(Constants::MENU_ID);
    menuModelEditor->menu()->setTitle(tr("Model Editor"));
    Core::ActionContainer *menuTools = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    menuTools->addMenu(menuModelEditor);

    Core::Command *exportDiagramCommand = registerCommand(
                Constants::EXPORT_DIAGRAM, [this]() { exportDiagram(); }, d->context, true,
                tr("Export Diagram..."));
    menuModelEditor->addAction(exportDiagramCommand);
    d->exportDiagramAction = exportDiagramCommand->action();

    menuModelEditor->addSeparator(d->context);

    Core::Command *zoomInCommand = registerCommand(
                Constants::ZOOM_IN, [this]() { zoomIn(); }, d->context, true,
                tr("Zoom In"), QKeySequence(QStringLiteral("Ctrl++")));
    menuModelEditor->addAction(zoomInCommand);
    d->zoomInAction = zoomInCommand->action();

    Core::Command *zoomOutCommand = registerCommand(
                Constants::ZOOM_OUT, [this]() { zoomOut(); }, d->context, true,
                tr("Zoom Out"), QKeySequence(QStringLiteral("Ctrl+-")));
    menuModelEditor->addAction(zoomOutCommand);
    d->zoomOutAction = zoomOutCommand->action();

    Core::Command *resetZoomCommand = registerCommand(
                Constants::RESET_ZOOM, [this]() { resetZoom(); }, d->context, true,
                tr("Reset Zoom"), QKeySequence(QStringLiteral("Ctrl+0")));
    menuModelEditor->addAction(resetZoomCommand);
    d->zoomOutAction = resetZoomCommand->action();

    d->openParentDiagramAction = registerCommand(
                Constants::OPEN_PARENT_DIAGRAM, [this]() { openParentDiagram(); }, Core::Context(), true,
                tr("Open Parent Diagram"), QKeySequence(QStringLiteral("Ctrl+Shift+P")))->action();
    d->openParentDiagramAction->setIcon(QIcon(QStringLiteral(":/modeleditor/up.png")));
    registerCommand(Constants::ACTION_ADD_PACKAGE, nullptr, Core::Context(), true, tr("Add Package"));
    registerCommand(Constants::ACTION_ADD_COMPONENT, nullptr, Core::Context(), true, tr("Add Component"));
    registerCommand(Constants::ACTION_ADD_CLASS, nullptr, Core::Context(), true, tr("Add Class"));
    registerCommand(Constants::ACTION_ADD_CANVAS_DIAGRAM, nullptr, Core::Context(), true, tr("Add Canvas Diagram"));
    d->synchronizeBrowserAction = registerCommand(
                Constants::ACTION_SYNC_BROWSER, nullptr, Core::Context(), true,
                tr("Synchronize Browser and Diagram<br><i><small>Press&Hold for options</small></i>"))->action();
    d->synchronizeBrowserAction->setIcon(Utils::Icons::LINK.icon());
    d->synchronizeBrowserAction->setCheckable(true);

    auto editPropertiesAction = new QAction(tr("Edit Element Properties"), Core::ICore::mainWindow());
    Core::Command *editPropertiesCommand = Core::ActionManager::registerAction(
                editPropertiesAction, Constants::SHORTCUT_MODEL_EDITOR_EDIT_PROPERTIES, d->context);
    editPropertiesCommand->setDefaultKeySequence(QKeySequence(tr("Shift+Return")));
    connect(editPropertiesAction, &QAction::triggered, this, &ActionHandler::onEditProperties);

    auto editItemAction = new QAction(tr("Edit Item on Diagram"), Core::ICore::mainWindow());
    Core::Command *editItemCommand = Core::ActionManager::registerAction(
                editItemAction, Constants::SHORTCUT_MODEL_EDITOR_EDIT_ITEM, d->context);
    editItemCommand->setDefaultKeySequence(QKeySequence(tr("Return")));
    connect(editItemAction, &QAction::triggered, this, &ActionHandler::onEditItem);
}

void ActionHandler::undo()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->undo();
}

void ActionHandler::redo()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->redo();
}

void ActionHandler::cut()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->cut();
}

void ActionHandler::copy()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->copy();
}

void ActionHandler::paste()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->paste();
}

void ActionHandler::removeSelectedElements()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->removeSelectedElements();
}

void ActionHandler::deleteSelectedElements()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->deleteSelectedElements();
}

void ActionHandler::selectAll()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->selectAll();
}

void ActionHandler::openParentDiagram()
{
    auto editor = dynamic_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->openParentDiagram();
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

void ActionHandler::exportDiagram()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->exportDiagram();
}

void ActionHandler::zoomIn()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->zoomIn();
}

void ActionHandler::zoomOut()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->zoomOut();
}

void ActionHandler::resetZoom()
{
    auto editor = qobject_cast<ModelEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->resetZoom();
}

Core::Command *ActionHandler::registerCommand(const Core::Id &id, const std::function<void()> &slot,
                                              const Core::Context &context, bool scriptable, const QString &title,
                                              const QKeySequence &keySequence)
{
    auto action = new QAction(title, this);
    Core::Command *command = Core::ActionManager::registerAction(action, id, context, scriptable);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(keySequence);
    if (slot)
        connect(action, &QAction::triggered, this, slot);
    return command;
}

} // namespace Internal
} // namespace ModelEditor
