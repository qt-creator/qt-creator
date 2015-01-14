/**************************************************************************
**
** Copyright (C) 2015 Dmitry Savchenko
** Copyright (C) 2015 Vasiliy Sorokin
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

#ifndef TODOOUTPUTPANE_H
#define TODOOUTPUTPANE_H

#include "settings.h"

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QToolButton;
class QButtonGroup;
class QModelIndex;
class QAbstractButton;
QT_END_NAMESPACE

namespace Todo {
namespace Internal {

class TodoItem;
class TodoItemsModel;
class TodoOutputTreeView;

class TodoOutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    TodoOutputPane(TodoItemsModel *todoItemsModel, QObject *parent = 0);
    ~TodoOutputPane();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets() const;
    QString displayName() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);
    void setFocus();
    bool hasFocus() const;
    bool canFocus() const;
    bool canNavigate() const;
    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();

    void setScanningScope(ScanningScope scanningScope);

signals:
    void todoItemClicked(const TodoItem &item);
    void scanningScopeChanged(ScanningScope scanningScope);

private slots:
    void scopeButtonClicked(QAbstractButton *button);
    void todoTreeViewClicked(const QModelIndex &index);
    void updateTodoCount();

private:
    TodoOutputTreeView *m_todoTreeView;
    QToolButton *m_currentFileButton;
    QToolButton *m_wholeProjectButton;
    QWidget *m_spacer;
    QButtonGroup *m_scopeButtons;
    QList<TodoItem> *items;
    TodoItemsModel *m_todoItemsModel;

    void createTreeView();
    void freeTreeView();
    void createScopeButtons();
    void freeScopeButtons();

    QModelIndex selectedModelIndex();
    QModelIndex nextModelIndex();
    QModelIndex previousModelIndex();
};

} // namespace Internal
} // namespace Todo

#endif // TODOOUTPUTPANE_H
