// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
