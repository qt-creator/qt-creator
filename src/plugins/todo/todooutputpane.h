/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Dmitry Savchenko.
** Copyright (c) 2010 Vasiliy Sorokin.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TODOOUTPUTPANE_H
#define TODOOUTPUTPANE_H

#include "settings.h"

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QTreeView;
class QToolButton;
class QButtonGroup;
class QModelIndex;
class QAbstractButton;
QT_END_NAMESPACE

namespace Todo {
namespace Internal {

class TodoItem;
class TodoItemsModel;

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

private:
    QTreeView *m_todoTreeView;
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
