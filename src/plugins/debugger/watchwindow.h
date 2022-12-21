// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/basetreeview.h>

namespace Debugger {
namespace Internal {

enum WatchType { LocalsType, InspectType, WatchersType, ReturnType, TooltipType };

class WatchTreeView : public Utils::BaseTreeView
{
    Q_OBJECT

public:
    explicit WatchTreeView(WatchType type);
    WatchType type() const { return m_type; }

    void setModel(QAbstractItemModel *model) override;
    void reset() override;

    static void reexpand(QTreeView *view, const QModelIndex &idx);

    void watchExpression(const QString &exp);
    void watchExpression(const QString &exp, const QString &name);
    void handleItemIsExpanded(const QModelIndex &idx);

signals:
    void currentIndexChanged(const QModelIndex &currentIndex);

private:
    void resetHelper();
    void expandNode(const QModelIndex &idx);
    void collapseNode(const QModelIndex &idx);
    void updateTimeColumn();
    void adjustSlider();

    void doItemsLayout() override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

    WatchType m_type;
    int m_sliderPosition = 0;
};

} // namespace Internal
} // namespace Debugger
