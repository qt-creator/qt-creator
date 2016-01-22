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

#include "callgrindnamedelegate.h"

#include "callgrindhelper.h"

#include <QApplication>
#include <QPainter>

namespace Valgrind {
namespace Internal {

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

} // namespace Internal
} // namespace Valgrind
