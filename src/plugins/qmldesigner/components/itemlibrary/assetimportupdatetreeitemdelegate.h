// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QItemDelegate>
#include <QStyleOptionViewItem>

namespace QmlDesigner {
namespace Internal {

class AssetImportUpdateTreeItemDelegate : public QItemDelegate
{
public:
    AssetImportUpdateTreeItemDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
private:
    struct LayoutInfo
    {
        QRect checkRect;
        QRect textRect;
        QRect iconRect;
        Qt::CheckState checkState;
        QStyleOptionViewItem option;
    };

    LayoutInfo getLayoutInfo(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void drawText(QPainter *painter, const QStyleOptionViewItem &option,
                  const QRect &rect, const QModelIndex &index) const;
};

} // namespace Internal
} // namespace QmlDesigner
