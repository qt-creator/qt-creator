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

#ifndef ACTIONHANDLER_H
#define ACTIONHANDLER_H

#include <QObject>

#include <coreplugin/icontext.h>

#include <functional>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {
class ActionContainer;
class Command;
}

namespace ModelEditor {
namespace Internal {

class ActionHandler :
        public QObject
{
    Q_OBJECT
    class ActionHandlerPrivate;

public:
    ActionHandler(const Core::Context &context, QObject *parent = 0);
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

    void createActions();
    void createEditPropertiesShortcut(const Core::Id &shortcutId);

private slots:
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void removeSelectedElements();
    void deleteSelectedElements();
    void selectAll();
    void openParentDiagram();
    void onEditProperties();

private:
    Core::Command *registerCommand(const Core::Id &id, const std::function<void()> &slot,
                                  bool scriptable = true, const QString &title = QString(),
                                  const QKeySequence &keySequence = QKeySequence());

private:
    ActionHandlerPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor

#endif // ACTIONHANDLER_H
