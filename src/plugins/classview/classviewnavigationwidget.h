// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    explicit NavigationWidget(QWidget *parent = nullptr);
    ~NavigationWidget() override;

    QList<QToolButton *> createToolButtons();

    bool flatMode() const;

    void setFlatMode(bool flatMode);

signals:
    void visibilityChanged(bool visibility);

    void requestGotoLocations(const QList<QVariant> &locations);

public:
    void onItemActivated(const QModelIndex &index);
    void onItemDoubleClicked(const QModelIndex &index);

    void onDataUpdate(QSharedPointer<QStandardItem> result);

    void onFullProjectsModeToggled(bool state);

protected:
    void fetchExpandedItems(QStandardItem *item, const QStandardItem *target) const;

    //! implements QWidget::hideEvent
    void hideEvent(QHideEvent *event) override;

    //! implements QWidget::showEvent
    void showEvent(QShowEvent *event) override;

private:
    Utils::NavigationTreeView *treeView;
    TreeItemModel *treeModel;
    QPointer<QToolButton> fullProjectsModeButton;
};

} // namespace Internal
} // namespace ClassView
