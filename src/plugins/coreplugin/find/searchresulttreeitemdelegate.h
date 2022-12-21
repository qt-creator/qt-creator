// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QItemDelegate>

namespace Core {
namespace Internal {

struct LayoutInfo
{
    QRect checkRect;
    QRect pixmapRect;
    QRect textRect;
    QRect lineNumberRect;
    QIcon icon;
    Qt::CheckState checkState;
    QStyleOptionViewItem option;
};

class SearchResultTreeItemDelegate: public QItemDelegate
{
public:
    SearchResultTreeItemDelegate(int tabWidth, QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setTabWidth(int width);
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
private:
    LayoutInfo getLayoutInfo(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    int drawLineNumber(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QModelIndex &index) const;
    void drawText(QPainter *painter, const QStyleOptionViewItem &option,
                           const QRect &rect, const QModelIndex &index) const;

    QString m_tabString;
};

} // namespace Internal
} // namespace Core
