/**************************************************************************
**
** This file is part of Qt Creator
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

#include "searchresulttreeitemdelegate.h"
#include "searchresulttreeitemroles.h"

#include <QModelIndex>
#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QDebug>

#include <math.h>

using namespace Find::Internal;

SearchResultTreeItemDelegate::SearchResultTreeItemDelegate(QObject *parent)
  : QItemDelegate(parent)
{
}

void SearchResultTreeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QStyleOptionViewItemV3 opt = setOptions(index, option);
    painter->setFont(opt.font);

    QItemDelegate::drawBackground(painter, opt, index);

    int iconAreaWidth = drawIcon(painter, opt, opt.rect, index);
    QRect resultRowRect(opt.rect.adjusted(iconAreaWidth, 0, 0, 0));

    int lineNumberAreaWidth = drawLineNumber(painter, opt, resultRowRect, index);
    resultRowRect.adjust(lineNumberAreaWidth, 0, 0, 0);

    QString displayString = index.model()->data(index, Qt::DisplayRole).toString();
    drawMarker(painter, index, displayString, resultRowRect);

    // Show number of subresults in displayString
    if (index.model()->hasChildren(index)) {
        displayString += QString::fromLatin1(" (")
                         + QString::number(index.model()->rowCount(index))
                         + QLatin1Char(')');
    }

    // Draw the text and focus/selection
    QItemDelegate::drawDisplay(painter, opt, resultRowRect, displayString);
    QItemDelegate::drawFocus(painter, opt, opt.rect);

    QVariant value = index.data(Qt::CheckStateRole);
    if (value.isValid()) {
        Qt::CheckState checkState = Qt::Unchecked;
        checkState = static_cast<Qt::CheckState>(value.toInt());
        QRect checkRect = check(opt, opt.rect, value);

        QRect emptyRect;
        doLayout(opt, &checkRect, &emptyRect, &emptyRect, false);

        QItemDelegate::drawCheck(painter, opt, checkRect, checkState);
    }

    painter->restore();
}

int SearchResultTreeItemDelegate::drawIcon(QPainter *painter, const QStyleOptionViewItemV3 &option,
                                                 const QRect &rect,
                                                 const QModelIndex &index) const
{
    static const int iconWidth = 16;
    static const int iconPadding = 4;
    QIcon icon = index.model()->data(index, ItemDataRoles::ResultIconRole).value<QIcon>();
    if (icon.isNull())
        return 0;
    QRect iconRect = rect.adjusted(iconPadding, 0, /*is set below anyhow*/0, 0);
    iconRect.setWidth(iconWidth);
    QItemDelegate::drawDecoration(painter, option, iconRect, icon.pixmap(iconWidth));
    return iconWidth + iconPadding;
}

int SearchResultTreeItemDelegate::drawLineNumber(QPainter *painter, const QStyleOptionViewItemV3 &option,
                                                 const QRect &rect,
                                                 const QModelIndex &index) const
{
    static const int lineNumberAreaHorizontalPadding = 4;
    int lineNumber = index.model()->data(index, ItemDataRoles::ResultLineNumberRole).toInt();
    if (lineNumber < 1)
        return 0;
    const bool isSelected = option.state & QStyle::State_Selected;
    int lineNumberDigits = (int)floor(log10((double)lineNumber)) + 1;
    int minimumLineNumberDigits = qMax((int)m_minimumLineNumberDigits, lineNumberDigits);
    int fontWidth = painter->fontMetrics().width(QString(minimumLineNumberDigits, QLatin1Char('0')));
    int lineNumberAreaWidth = lineNumberAreaHorizontalPadding + fontWidth + lineNumberAreaHorizontalPadding;
    QRect lineNumberAreaRect(rect);
    lineNumberAreaRect.setWidth(lineNumberAreaWidth);

    QPalette::ColorGroup cg = QPalette::Normal;
    if (!(option.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    else if (!(option.state & QStyle::State_Enabled))
        cg = QPalette::Disabled;

    painter->fillRect(lineNumberAreaRect, QBrush(isSelected ?
        option.palette.brush(cg, QPalette::Highlight) :
        option.palette.color(cg, QPalette::Base).darker(111)));

    QStyleOptionViewItemV3 opt = option;
    opt.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
    opt.palette.setColor(cg, QPalette::Text, Qt::darkGray);

    const QStyle *style = QApplication::style();
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, 0) + 1;

    const QRect rowRect = lineNumberAreaRect.adjusted(-textMargin, 0, textMargin-lineNumberAreaHorizontalPadding, 0);
    QItemDelegate::drawDisplay(painter, opt, rowRect, QString::number(lineNumber));

    return lineNumberAreaWidth;
}

void SearchResultTreeItemDelegate::drawMarker(QPainter *painter, const QModelIndex &index, const QString text,
                                              const QRect &rect) const
{
    int searchTermStart = index.model()->data(index, ItemDataRoles::SearchTermStartRole).toInt();
    int searchTermLength = index.model()->data(index, ItemDataRoles::SearchTermLengthRole).toInt();
    if (searchTermStart < 0 || searchTermLength < 1)
        return;
    const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    int searchTermStartPixels = painter->fontMetrics().width(text.left(searchTermStart));
    int searchTermLengthPixels = painter->fontMetrics().width(text.mid(searchTermStart, searchTermLength));
    QRect resultHighlightRect(rect);
    resultHighlightRect.setLeft(resultHighlightRect.left() + searchTermStartPixels + textMargin - 1); // -1: Cosmetics
    resultHighlightRect.setRight(resultHighlightRect.left() + searchTermLengthPixels + 1); // +1: Cosmetics
    painter->fillRect(resultHighlightRect, QBrush(qRgb(255, 240, 120)));
}
