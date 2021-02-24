/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "assetimportupdatetreeitemdelegate.h"
#include "assetimportupdatetreemodel.h"

#include <QPainter>
#include <QModelIndex>

namespace QmlDesigner {
namespace Internal {

AssetImportUpdateTreeItemDelegate::AssetImportUpdateTreeItemDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

AssetImportUpdateTreeItemDelegate::LayoutInfo AssetImportUpdateTreeItemDelegate::getLayoutInfo(
        const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    LayoutInfo info;
    info.option = setOptions(index, option);

    const bool checkable = (index.model()->flags(index) & Qt::ItemIsUserCheckable);
    info.checkState = Qt::Unchecked;
    if (checkable) {
        QVariant checkStateData = index.data(Qt::CheckStateRole);
        info.checkState = static_cast<Qt::CheckState>(checkStateData.toInt());
        info.checkRect = doCheck(info.option, info.option.rect, checkStateData);
    }

    info.textRect = info.option.rect.adjusted(0, 0, info.checkRect.width(), 0);

    doLayout(info.option, &info.checkRect, &info.iconRect, &info.textRect, false);

    return info;
}

void AssetImportUpdateTreeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                              const QModelIndex &index) const
{
    painter->save();

    const LayoutInfo info = getLayoutInfo(option, index);

    painter->setFont(info.option.font);

    drawBackground(painter, info.option, index);
    drawText(painter, info.option, info.textRect, index);
    drawCheck(painter, info.option, info.checkRect, info.checkState);

    painter->restore();
}

QSize AssetImportUpdateTreeItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                                  const QModelIndex &index) const
{
    const LayoutInfo info = getLayoutInfo(option, index);
    const int height = index.data(Qt::SizeHintRole).value<QSize>().height();
    // get text width, see QItemDelegatePrivate::displayRect
    const QString text = index.data(Qt::DisplayRole).toString();
    const QRect textMaxRect(0, 0, INT_MAX / 256, height);
    const QRect textLayoutRect = textRectangle(nullptr, textMaxRect, info.option.font, text);
    const QRect textRect(info.textRect.x(), info.textRect.y(), textLayoutRect.width(), height);
    const QRect layoutRect = info.checkRect | textRect;
    return QSize(layoutRect.x(), layoutRect.y()) + layoutRect.size();
}

void AssetImportUpdateTreeItemDelegate::drawText(QPainter *painter,
                                                 const QStyleOptionViewItem &option,
                                                 const QRect &rect,
                                                 const QModelIndex &index) const
{
    const QString text = index.data(Qt::DisplayRole).toString();
    drawDisplay(painter, option, rect, text);
}

} // namespace Internal
} // namespace QmlDesigner
