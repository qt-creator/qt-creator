// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/inavigationwidgetfactory.h>

#include <utils/navigationtreeview.h>

QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

namespace Squish {
namespace Internal {

class SquishTestTreeModel;
class SquishTestTreeSortModel;
class SquishTestTreeView;

class SquishNavigationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SquishNavigationWidget(QWidget *parent = nullptr);
    ~SquishNavigationWidget() override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    static QList<QToolButton *> createToolButtons();

private:
    void onItemActivated(const QModelIndex &idx);
    void onExpanded(const QModelIndex &idx);
    void onCollapsed(const QModelIndex &idx);
    void onRowsInserted(const QModelIndex &parent, int, int);
    void onRowsRemoved(const QModelIndex &parent, int, int);
    void onRemoveSharedFolderTriggered(int row, const QModelIndex &parent);
    void onRemoveAllSharedFolderTriggered();
    void onRecordTestCase(const QString &suiteName, const QString &testCase);
    void onNewTestCaseTriggered(const QModelIndex &index);

    SquishTestTreeView *m_view;
    SquishTestTreeModel *m_model; // not owned
    SquishTestTreeSortModel *m_sortModel;
};

class SquishNavigationWidgetFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    SquishNavigationWidgetFactory();

private:
    Core::NavigationView createWidget() override;
};

} // namespace Internal
} // namespace Squish
