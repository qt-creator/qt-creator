/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "searchresulttreeitemdelegate.h"
#include "searchresulttreeitemroles.h"

#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QApplication>

#include <QModelIndex>
#include <QDebug>

#include <math.h>

using namespace Find::Internal;

SearchResultTreeItemDelegate::SearchResultTreeItemDelegate(QObject *parent)
  : QItemDelegate(parent)
{
}

void SearchResultTreeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    static const int iconSize = 16;

    painter->save();

    QStyleOptionViewItemV3 opt = setOptions(index, option);
    painter->setFont(opt.font);

    QItemDelegate::drawBackground(painter, opt, index);

    // ---- do the layout
    QRect checkRect;
    QRect pixmapRect;
    QRect textRect;

    // check mark
    bool checkable = (index.model()->flags(index) & Qt::ItemIsUserCheckable);
    Qt::CheckState checkState = Qt::Unchecked;
    if (checkable) {
        QVariant checkStateData = index.data(Qt::CheckStateRole);
        checkState = static_cast<Qt::CheckState>(checkStateData.toInt());
#if QT_VERSION >= 0x050000
        checkRect = doCheck(opt, opt.rect, checkStateData);
#else // Qt4
        checkRect = check(opt, opt.rect, checkStateData);
#endif
    }

    // icon
    QIcon icon = index.model()->data(index, ItemDataRoles::ResultIconRole).value<QIcon>();
    if (!icon.isNull()) {
        pixmapRect = QRect(0, 0, iconSize, iconSize);
    }

    // text
    textRect = opt.rect.adjusted(0, 0, checkRect.width() + pixmapRect.width(), 0);

    // do layout
    doLayout(opt, &checkRect, &pixmapRect, &textRect, false);

    // ---- draw the items
    // icon
    if (!icon.isNull())
        QItemDelegate::drawDecoration(painter, opt, pixmapRect, icon.pixmap(iconSize));

    // line numbers
    int lineNumberAreaWidth = drawLineNumber(painter, opt, textRect, index);
    textRect.adjust(lineNumberAreaWidth, 0, 0, 0);

    // selected text
    QString displayString = index.model()->data(index, Qt::DisplayRole).toString();
    drawMarker(painter, index, displayString, textRect);

    // show number of subresults in displayString
    if (index.model()->hasChildren(index)) {
        displayString += QString::fromLatin1(" (")
                         + QString::number(index.model()->rowCount(index))
                         + QLatin1Char(')');
    }

    // text and focus/selection
    QItemDelegate::drawDisplay(painter, opt, textRect, displayString);
    QItemDelegate::drawFocus(painter, opt, opt.rect);

    // check mark
    if (checkable)
        QItemDelegate::drawCheck(painter, opt, checkRect, checkState);

    painter->restore();
}

// returns the width of the line number area
int SearchResultTreeItemDelegate::drawLineNumber(QPainter *painter, const QStyleOptionViewItemV3 &option,
                                                 const QRect &rect,
                                                 const QModelIndex &index) const
{
    static const int lineNumberAreaHorizontalPadding = 4;
    int lineNumber = index.model()->data(index, ItemDataRoles::ResultLineNumberRole).toInt();
    if (lineNumber < 1)
        return 0;
    const bool isSelected = option.state & QStyle::State_Selected;
    QString lineText = QString::number(lineNumber);
    int minimumLineNumberDigits = qMax((int)m_minimumLineNumberDigits, lineText.count());
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
    QItemDelegate::drawDisplay(painter, opt, rowRect, lineText);

    return lineNumberAreaWidth;
}

void SearchResultTreeItemDelegate::drawMarker(QPainter *painter, const QModelIndex &index, const QString text,
                                              const QRect &rect) const
{
    int searchTermStart = index.model()->data(index, ItemDataRoles::SearchTermStartRole).toInt();
    int searchTermLength = index.model()->data(index, ItemDataRoles::SearchTermLengthRole).toInt();
    if (searchTermStart < 0 || searchTermStart >= text.length() || searchTermLength < 1)
        return;
    // clip searchTermLength to end of line
    searchTermLength = qMin(searchTermLength, text.length() - searchTermStart);
    const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    int searchTermStartPixels = painter->fontMetrics().width(text.left(searchTermStart));
    int searchTermLengthPixels = painter->fontMetrics().width(text.mid(searchTermStart, searchTermLength));
    QRect resultHighlightRect(rect);
    resultHighlightRect.setLeft(resultHighlightRect.left() + searchTermStartPixels + textMargin - 1); // -1: Cosmetics
    resultHighlightRect.setRight(resultHighlightRect.left() + searchTermLengthPixels + 1); // +1: Cosmetics
    painter->fillRect(resultHighlightRect, QBrush(qRgb(255, 240, 120)));
}
