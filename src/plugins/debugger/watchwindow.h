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

    void setModel(QAbstractItemModel *model);
    void reset();

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
    void adjustSlider();

    void doItemsLayout();
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

    WatchType m_type;
    int m_sliderPosition;
};

} // namespace Internal
} // namespace Debugger
