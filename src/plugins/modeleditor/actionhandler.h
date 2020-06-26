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

#pragma once

#include <coreplugin/icontext.h>

#include <QIcon>
#include <QObject>

#include <functional>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {
class Command;
}

namespace ModelEditor {
namespace Internal {

class ModelEditor;

class ActionHandler :
        public QObject
{
    Q_OBJECT
    class ActionHandlerPrivate;

public:
    ActionHandler();
    ~ActionHandler();

public:
    QAction *undoAction() const;
    QAction *redoAction() const;
    QAction *cutAction() const;
    QAction *copyAction() const;
    QAction *pasteAction() const;
    QAction *removeAction() const;
    QAction *deleteAction() const;
    QAction *selectAllAction() const;
    QAction *openParentDiagramAction() const;
    QAction *synchronizeBrowserAction() const;
    QAction *exportDiagramAction() const;
    QAction *exportSelectedElementsAction() const;
    QAction *zoomInAction() const;
    QAction *zoomOutAction() const;
    QAction *resetZoom() const;

    void createActions();

private:
    void onEditProperties();
    void onEditItem();

    Core::Command *registerCommand(const Utils::Id &id, void (ModelEditor::*function)(),
                                   const Core::Context &context, const QString &title = QString(),
                                   const QKeySequence &keySequence = QKeySequence(),
                                   const QIcon &icon = QIcon());

private:
    ActionHandlerPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
