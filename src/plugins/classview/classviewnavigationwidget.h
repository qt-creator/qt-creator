/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov
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

#include "classviewtreeitemmodel.h"

#include <QList>
#include <QPointer>
#include <QSharedPointer>
#include <QStandardItem>
#include <QToolButton>
#include <QWidget>

namespace Utils { class NavigationTreeView; }

namespace ClassView {
namespace Internal {

class NavigationWidgetPrivate;

class NavigationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NavigationWidget(QWidget *parent = 0);
    ~NavigationWidget();

    QList<QToolButton *> createToolButtons();

    bool flatMode() const;

    void setFlatMode(bool flatMode);

signals:
    void visibilityChanged(bool visibility);

    void requestGotoLocation(const QString &name, int line, int column);

    void requestGotoLocations(const QList<QVariant> &locations);

    void requestTreeDataUpdate();

public:
    void onItemActivated(const QModelIndex &index);
    void onItemDoubleClicked(const QModelIndex &index);

    void onDataUpdate(QSharedPointer<QStandardItem> result);

    void onFullProjectsModeToggled(bool state);

protected:
    void fetchExpandedItems(QStandardItem *item, const QStandardItem *target) const;

    //! implements QWidget::hideEvent
    void hideEvent(QHideEvent *event);

    //! implements QWidget::showEvent
    void showEvent(QShowEvent *event);

private:
    Utils::NavigationTreeView *treeView;
    TreeItemModel *treeModel;
    QPointer<QToolButton> fullProjectsModeButton;
};

} // namespace Internal
} // namespace ClassView
