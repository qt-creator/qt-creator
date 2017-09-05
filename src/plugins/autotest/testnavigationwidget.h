/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "testrunner.h"

#include <coreplugin/inavigationwidgetfactory.h>

#include <utils/navigationtreeview.h>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QTimer;
class QToolButton;
QT_END_NAMESPACE

namespace Core {
class IContext;
}

namespace Utils {
class ProgressIndicator;
}

namespace Autotest {
namespace Internal {

class TestTreeModel;
class TestTreeSortFilterModel;
class TestTreeView;

class TestNavigationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TestNavigationWidget(QWidget *parent = 0);
    ~TestNavigationWidget();
    void contextMenuEvent(QContextMenuEvent *event) override;
    QList<QToolButton *> createToolButtons();

signals:

private:
    void onItemActivated(const QModelIndex &index);
    void onSortClicked();
    void onFilterMenuTriggered(QAction *action);
    void onParsingStarted();
    void onParsingFinished();
    void initializeFilterMenu();
    void onRunThisTestTriggered(TestRunMode runMode);

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
