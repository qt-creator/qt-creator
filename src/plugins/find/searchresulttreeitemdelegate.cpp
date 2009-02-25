/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "searchresulttreeitemdelegate.h"
#include "searchresulttreeitemroles.h"

#include <QModelIndex>
#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QApplication>

#include <math.h>

using namespace Find::Internal;

SearchResultTreeItemDelegate::SearchResultTreeItemDelegate(QObject *parent)
  : QItemDelegate(parent)
{
}

void SearchResultTreeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.model()->data(index, ItemDataRoles::TypeRole).toString().compare("file") == 0) {
        QItemDelegate::paint(painter, option, index);
    } else {
        painter->save();

        QStyleOptionViewItemV3 opt = setOptions(index, option);
        painter->setFont(opt.font);

        QItemDelegate::drawBackground(painter, opt, index);

        int lineNumberAreaWidth = drawLineNumber(painter, opt, index);

        QRect resultRowRect(opt.rect.adjusted(lineNumberAreaWidth, 0, 0, 0));
        QString displayString = index.model()->data(index, Qt::DisplayRole).toString();
        drawMarker(painter, index, displayString, resultRowRect);

        // Draw the text and focus/selection
        QItemDelegate::drawDisplay(painter, opt, resultRowRect, displayString);
        QItemDelegate::drawFocus(painter, opt, opt.rect);

        painter->restore();
    }
}

int SearchResultTreeItemDelegate::drawLineNumber(QPainter *painter, const QStyleOptionViewItemV3 &option,
    const QModelIndex &index) const
{
    static const int lineNumberAreaHorizontalPadding = 4;
    const bool isSelected = option.state & QStyle::State_Selected;
    int lineNumber = index.model()->data(index, ItemDataRoles::ResultLineNumberRole).toInt();
    int lineNumberDigits = (int)floor(log10((double)lineNumber)) + 1;
    int minimumLineNumberDigits = qMax((int)m_minimumLineNumberDigits, lineNumberDigits);
    int fontWidth = painter->fontMetrics().width(QString(minimumLineNumberDigits, '0'));
    int lineNumberAreaWidth = lineNumberAreaHorizontalPadding + fontWidth + lineNumberAreaHorizontalPadding;
    QRect lineNumberAreaRect(option.rect);
    lineNumberAreaRect.setWidth(lineNumberAreaWidth);

    QPalette::ColorGroup cg = QPalette::Normal;
    if (!(option.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    else if (!(option.state & QStyle::State_Enabled))
        cg = QPalette::Disabled;

    painter->fillRect(lineNumberAreaRect, QBrush(isSelected?
        option.palette.brush(cg, QPalette::Highlight):QBrush(qRgb(230, 230, 230))));
    painter->setPen(isSelected?
        option.palette.color(cg, QPalette::HighlightedText):Qt::darkGray);
    painter->drawText(lineNumberAreaRect.adjusted(0, 0, -lineNumberAreaHorizontalPadding, 0),
        Qt::AlignRight, QString::number(lineNumber));

    return lineNumberAreaWidth;
}

void SearchResultTreeItemDelegate::drawMarker(QPainter *painter, const QModelIndex &index, const QString text,
    const QRect &rect) const
{
    const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    int searchTermStart = index.model()->data(index, ItemDataRoles::SearchTermStartRole).toInt();
    int searchTermStartPixels = painter->fontMetrics().width(text.left(searchTermStart));
    int searchTermLength = index.model()->data(index, ItemDataRoles::SearchTermLengthRole).toInt();
    int searchTermLengthPixels = painter->fontMetrics().width(text.mid(searchTermStart, searchTermLength));
    QRect resultHighlightRect(rect);
    resultHighlightRect.setLeft(resultHighlightRect.left() + searchTermStartPixels + textMargin - 1); // -1: Cosmetics
    resultHighlightRect.setRight(resultHighlightRect.left() + searchTermLengthPixels + 1); // +1: Cosmetics
    painter->fillRect(resultHighlightRect, QBrush(qRgb(255, 240, 120)));
}
