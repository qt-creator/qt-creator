/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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
