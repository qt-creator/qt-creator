/**************************************************************************
**
** This file is part of Qt Creator Analyzer Tools
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "callgrindnamedelegate.h"

#include "callgrindhelper.h"

#include <QApplication>
#include <QPainter>

namespace Callgrind {
namespace Internal {

NameDelegate::NameDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}

NameDelegate::~NameDelegate()
{
}

void NameDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    // init
    QStyleOptionViewItemV4 opt(option);
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

} // Internal
} // Callgrind
