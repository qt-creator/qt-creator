// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itemdatacache.h"

#include <coreplugin/inavigationwidgetfactory.h>

#include <utils/navigationtreeview.h>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QTimer;
class QToolButton;
QT_END_NAMESPACE

namespace Utils {
class ProgressIndicator;
}

namespace Autotest {

class TestTreeModel;

namespace Internal {

class TestRunner;
class TestTreeSortFilterModel;
class TestTreeView;

class TestNavigationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TestNavigationWidget(QWidget *parent = nullptr);
    void contextMenuEvent(QContextMenuEvent *event) override;
    QList<QToolButton *> createToolButtons();

    void updateExpandedStateCache();

private:
    void onItemActivated(const QModelIndex &index);
    void onSortClicked();
    void onFilterMenuTriggered(QAction *action);
    void onParsingStarted();
    void onParsingFinished();
    void initializeFilterMenu();
    void onRunThisTestTriggered(TestRunMode runMode);
    void reapplyCachedExpandedState();

    TestTreeModel *m_model;
    TestTreeSortFilterModel *m_sortFilterModel;
    TestTreeView *m_view;
    QToolButton *m_sort;
    QToolButton *m_filterButton;
    QMenu *m_filterMenu;
    bool m_sortAlphabetically;
    Utils::ProgressIndicator *m_progressIndicator;
    QTimer *m_progressTimer;
    QFrame *m_missingFrameworksWidget;
    ItemDataCache<bool> m_expandedStateCache;
};

class TestNavigationWidgetFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    TestNavigationWidgetFactory();

private:
    Core::NavigationView createWidget() override;

};

} // namespace Internal
} // namespace Autotest
