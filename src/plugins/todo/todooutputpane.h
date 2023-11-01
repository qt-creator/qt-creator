// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "settings.h"

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QToolButton;
class QButtonGroup;
class QModelIndex;
class QAbstractButton;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Todo {
namespace Internal {

class TodoItem;
class TodoItemsModel;
class TodoOutputTreeView;

using QToolButtonList = QList<QToolButton *>;

class TodoOutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    TodoOutputPane(TodoItemsModel *todoItemsModel, const Settings *settings, QObject *parent = nullptr);
    ~TodoOutputPane() override;

    QWidget *outputWidget(QWidget *parent) override;
    QList<QWidget*> toolBarWidgets() const override;
    void clearContents() override;
    void setFocus() override;
    bool hasFocus() const override;
    bool canFocus() const override;
    bool canNavigate() const override;
    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;

    void setScanningScope(ScanningScope scanningScope);

signals:
    void todoItemClicked(const TodoItem &item);
    void scanningScopeChanged(ScanningScope scanningScope);

private:
    void scopeButtonClicked(QAbstractButton *button);
    void todoTreeViewClicked(const QModelIndex &index);
    void updateTodoCount();
    void updateKeywordFilter();
    void clearKeywordFilter();

    void createTreeView();
    void freeTreeView();
    void createScopeButtons();
    void freeScopeButtons();

    QModelIndex selectedModelIndex();
    QModelIndex nextModelIndex();
    QModelIndex previousModelIndex();

    QToolButton *createCheckableToolButton(const QString &text, const QString &toolTip, const QIcon &icon);

    TodoOutputTreeView *m_todoTreeView;
    QToolButton *m_currentFileButton;
    QToolButton *m_wholeProjectButton;
    QToolButton *m_subProjectButton;
    QWidget *m_spacer;
    QButtonGroup *m_scopeButtons;
    QList<TodoItem> *items;
    TodoItemsModel *m_todoItemsModel;
    QSortFilterProxyModel *m_filteredTodoItemsModel;
    const Settings *m_settings;
    QToolButtonList m_filterButtons;
};

} // namespace Internal
} // namespace Todo
