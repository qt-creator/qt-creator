/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#include "actionhandler.h"

#include "modeleditor_constants.h"
#include "abstracteditor.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <QAction>
#include <QShortcut>

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

void ActionHandler::createActions()
{
    Core::ActionContainer *medit = Core::ActionManager::actionContainer(Core::Constants::M_EDIT);

    d->undoAction = registerCommand(Core::Constants::UNDO, [this]() { undo(); })->action();
    d->redoAction = registerCommand(Core::Constants::REDO, [this]() { redo(); })->action();
    d->cutAction = registerCommand(Core::Constants::CUT, [this]() { cut(); })->action();
    d->copyAction = registerCommand(Core::Constants::COPY, [this]() { copy(); })->action();
    d->pasteAction = registerCommand(Core::Constants::PASTE, [this]() { paste(); })->action();
    Core::Command *removeCommand = registerCommand(
                Constants::REMOVE_SELECTED_ELEMENTS, [this]() { removeSelectedElements(); }, true,
                tr("&Remove"), QKeySequence::Delete);
    medit->addAction(removeCommand, Core::Constants::G_EDIT_COPYPASTE);
    d->removeAction = removeCommand->action();
    Core::Command *deleteCommand = registerCommand(
                Constants::DELETE_SELECTED_ELEMENTS, [this]() { deleteSelectedElements(); }, true,
                tr("&Delete"), QKeySequence(QStringLiteral("Ctrl+D")));
    medit->addAction(deleteCommand, Core::Constants::G_EDIT_COPYPASTE);
    d->deleteAction = deleteCommand->action();
    d->selectAllAction = registerCommand(Core::Constants::SELECTALL, [this]() { selectAll(); })->action();
    registerCommand(Constants::ACTION_ADD_PACKAGE, nullptr);
    registerCommand(Constants::ACTION_ADD_COMPONENT, nullptr);
    registerCommand(Constants::ACTION_ADD_CLASS, nullptr);
    registerCommand(Constants::ACTION_ADD_CANVAS_DIAGRAM, nullptr);
}

void ActionHandler::createEditPropertiesShortcut(const Core::Id &shortcutId)
{
    auto editAction = new QAction(tr("Edit selected element in properties view"), Core::ICore::mainWindow());
    Core::Command *editCommand = Core::ActionManager::registerAction(
                editAction, shortcutId, d->context);
    editCommand->setDefaultKeySequence(QKeySequence(tr("F2")));
    connect(editAction, &QAction::triggered, this, &ActionHandler::onEditProperties);
}

void ActionHandler::undo()
{
    auto editor = dynamic_cast<AbstractEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->undo();
}

void ActionHandler::redo()
{
    auto editor = dynamic_cast<AbstractEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->redo();
}

void ActionHandler::cut()
{
    auto editor = dynamic_cast<AbstractEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->cut();
}

void ActionHandler::copy()
{
    auto editor = dynamic_cast<AbstractEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->copy();
}

void ActionHandler::paste()
{
    auto editor = dynamic_cast<AbstractEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->paste();
}

void ActionHandler::removeSelectedElements()
{
    auto editor = dynamic_cast<AbstractEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->removeSelectedElements();
}

void ActionHandler::deleteSelectedElements()
{
    auto editor = dynamic_cast<AbstractEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->deleteSelectedElements();
}

void ActionHandler::selectAll()
{
    auto editor = dynamic_cast<AbstractEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->selectAll();
}

void ActionHandler::onEditProperties()
{
    auto editor = dynamic_cast<AbstractEditor *>(Core::EditorManager::currentEditor());
    if (editor)
        editor->editProperties();
}

Core::Command *ActionHandler::registerCommand(const Core::Id &id, const std::function<void()> &slot,
                                              bool scriptable, const QString &title,
                                              const QKeySequence &keySequence)
{
    auto action = new QAction(title, this);
    Core::Command *command = Core::ActionManager::registerAction(action, id, d->context, scriptable);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(keySequence);
    if (slot)
        connect(action, &QAction::triggered, this, slot);
    return command;
}

} // namespace Internal
} // namespace ModelEditor
