// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindnamedelegate.h"

#include "callgrindhelper.h"

#include <QApplication>
#include <QPainter>

namespace Valgrind::Internal {

NameDelegate::NameDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void NameDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    // init
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const int margin = 2;
    const int size = 10;

    const QString text = index.data().toString();
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

    // draw controls, but no text
    opt.text.clear();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter);

    // draw bar in the first few pixels
    painter->save();
    const QRectF barRect = opt.rect.adjusted(
            margin, margin, -opt.rect.width() + size - margin, -margin);
    painter->setPen(Qt::black);
    painter->setBrush(CallgrindHelper::colorForString(text));
    painter->drawRect(barRect);

    // move cell rect to right
    opt.rect.adjust(size+margin, 0, 0, 0);

    // draw text
    const QString elidedText = painter->fontMetrics().elidedText(text, Qt::ElideRight,
                                                                 opt.rect.width());

    const QBrush &textBrush = (option.state & QStyle::State_Selected)
                                ? opt.palette.highlightedText()
                                : opt.palette.text();
    painter->setBrush(Qt::NoBrush);
    painter->setPen(textBrush.color());
    painter->drawText(opt.rect, elidedText);
    painter->restore();
}

} // namespace Valgrind::Internal
