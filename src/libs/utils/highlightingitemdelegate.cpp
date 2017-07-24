/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "highlightingitemdelegate.h"

#include <QApplication>
#include <QModelIndex>
#include <QPainter>

const int kMinimumLineNumberDigits = 6;

namespace Utils {

HighlightingItemDelegate::HighlightingItemDelegate(int tabWidth, QObject *parent)
    : QItemDelegate(parent)
{
    setTabWidth(tabWidth);
}

void HighlightingItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
    static const int iconSize = 16;

    painter->save();

    const QStyleOptionViewItem opt = setOptions(index, option);
    painter->setFont(opt.font);

    QItemDelegate::drawBackground(painter, opt, index);

    // ---- do the layout
    QRect checkRect;
    QRect pixmapRect;
    QRect textRect;

    // check mark
    const bool checkable = (index.model()->flags(index) & Qt::ItemIsUserCheckable);
    Qt::CheckState checkState = Qt::Unchecked;
    if (checkable) {
        QVariant checkStateData = index.data(Qt::CheckStateRole);
        checkState = static_cast<Qt::CheckState>(checkStateData.toInt());
        checkRect = doCheck(opt, opt.rect, checkStateData);
    }

    // icon
    const QIcon icon = index.model()->data(index, Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull()) {
        const QSize size = icon.actualSize(QSize(iconSize, iconSize));
        pixmapRect = QRect(0, 0, size.width(), size.height());
    }

    // text
    textRect = opt.rect.adjusted(0, 0, checkRect.width() + pixmapRect.width(), 0);

    // do layout
    doLayout(opt, &checkRect, &pixmapRect, &textRect, false);
    // ---- draw the items
    // icon
    if (!icon.isNull())
        icon.paint(painter, pixmapRect, option.decorationAlignment);

    // line numbers
    const int lineNumberAreaWidth = drawLineNumber(painter, opt, textRect, index);
    textRect.adjust(lineNumberAreaWidth, 0, 0, 0);

    // text and focus/selection
    drawText(painter, opt, textRect, index);
    QItemDelegate::drawFocus(painter, opt, opt.rect);

    // check mark
    if (checkable)
        QItemDelegate::drawCheck(painter, opt, checkRect, checkState);

    painter->restore();
}

void HighlightingItemDelegate::setTabWidth(int width)
{
    m_tabString = QString(width, ' ');
}

// returns the width of the line number area
int HighlightingItemDelegate::drawLineNumber(QPainter *painter, const QStyleOptionViewItem &option,
                                             const QRect &rect,
                                             const QModelIndex &index) const
{
    static const int lineNumberAreaHorizontalPadding = 4;
    const int lineNumber = index.model()->data(index, int(HighlightingItemRole::LineNumber)).toInt();
    if (lineNumber < 1)
        return 0;
    const bool isSelected = option.state & QStyle::State_Selected;
    const QString lineText = QString::number(lineNumber);
    const int minimumLineNumberDigits = qMax(kMinimumLineNumberDigits, lineText.count());
    const int fontWidth = painter->fontMetrics().width(QString(minimumLineNumberDigits, '0'));
    const int lineNumberAreaWidth = lineNumberAreaHorizontalPadding + fontWidth
                                    + lineNumberAreaHorizontalPadding;
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

    QStyleOptionViewItem opt = option;
    opt.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
    opt.palette.setColor(cg, QPalette::Text, Qt::darkGray);

    const QStyle *style = QApplication::style();
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, 0) + 1;

    const QRect rowRect
            = lineNumberAreaRect.adjusted(-textMargin, 0,
                                          textMargin - lineNumberAreaHorizontalPadding, 0);
    QItemDelegate::drawDisplay(painter, opt, rowRect, lineText);

    return lineNumberAreaWidth;
}

void HighlightingItemDelegate::drawText(QPainter *painter,
                                        const QStyleOptionViewItem &option,
                                        const QRect &rect,
                                        const QModelIndex &index) const
{
    QString text = index.model()->data(index, Qt::DisplayRole).toString();
    // show number of subresults in displayString
    if (index.model()->hasChildren(index))
        text += " (" + QString::number(index.model()->rowCount(index)) + ')';

    QVector<int> searchTermStarts =
            index.model()->data(index, int(HighlightingItemRole::StartColumn)).value<QVector<int>>();
    QVector<int> searchTermLengths =
            index.model()->data(index, int(HighlightingItemRole::Length)).value<QVector<int>>();

    if (searchTermStarts.isEmpty()) {
        drawDisplay(painter, option, rect, text.replace('\t', m_tabString), {});
        return;
    }

    // replace tabs with searchTerm bookkeeping
    const int tabDiff = m_tabString.size() - 1;
    for (int i = 0; i < text.length(); i++) {
        if (text.at(i) != '\t')
            continue;

        text.replace(i, 1, m_tabString);

        // adjust highlighting length if tab is highlighted
        for (int j = 0; j < searchTermStarts.size(); ++j) {
            if (i >= searchTermStarts.at(j) && i < searchTermStarts.at(j) + searchTermLengths.at(j))
                searchTermLengths[j] += tabDiff;
        }

        // adjust all following highlighting starts
        for (int j = 0; j < searchTermStarts.size(); ++j) {
            if (searchTermStarts.at(j) > i)
                searchTermStarts[j] += tabDiff;
        }

        i += tabDiff;
    }

    const QColor highlightForeground =
            index.model()->data(index, int(HighlightingItemRole::Foreground)).value<QColor>();
    const QColor highlightBackground =
            index.model()->data(index, int(HighlightingItemRole::Background)).value<QColor>();
    QTextCharFormat highlightFormat;
    highlightFormat.setForeground(highlightForeground);
    highlightFormat.setBackground(highlightBackground);

    QVector<QTextLayout::FormatRange> formats;
    for (int i = 0, size = searchTermStarts.size(); i < size; ++i)
        formats.append({searchTermStarts.at(i), searchTermLengths.at(i), highlightFormat});

    drawDisplay(painter, option, rect, text, formats);
}

// copied from QItemDelegate for drawDisplay
static QString replaceNewLine(QString text)
{
    static const QChar nl = '\n';
    for (int i = 0; i < text.count(); ++i)
        if (text.at(i) == nl)
            text[i] = QChar::LineSeparator;
    return text;
}

// copied from QItemDelegate for drawDisplay
QSizeF doTextLayout(QTextLayout *textLayout, int lineWidth)
{
    qreal height = 0;
    qreal widthUsed = 0;
    textLayout->beginLayout();
    while (true) {
        QTextLine line = textLayout->createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout->endLayout();
    return QSizeF(widthUsed, height);
}

// copied from QItemDelegate to be able to add the 'format' parameter
void HighlightingItemDelegate::drawDisplay(QPainter *painter,
                                           const QStyleOptionViewItem &option,
                                           const QRect &rect, const QString &text,
                                           const QVector<QTextLayout::FormatRange> &format) const
{
    QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                              ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, option.palette.brush(cg, QPalette::Highlight));
        painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
    } else {
        painter->setPen(option.palette.color(cg, QPalette::Text));
    }

    if (text.isEmpty())
        return;

    if (option.state & QStyle::State_Editing) {
        painter->save();
        painter->setPen(option.palette.color(cg, QPalette::Text));
        painter->drawRect(rect.adjusted(0, 0, -1, -1));
        painter->restore();
    }

    const QStyleOptionViewItem opt = option;

    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, widget) + 1;
    QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
    const bool wrapText = opt.features & QStyleOptionViewItem::WrapText;
    QTextOption textOption;
    textOption.setWrapMode(wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap);
    textOption.setTextDirection(option.direction);
    textOption.setAlignment(QStyle::visualAlignment(option.direction, option.displayAlignment));
    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setFont(option.font);
    textLayout.setText(replaceNewLine(text));

    QSizeF textLayoutSize = doTextLayout(&textLayout, textRect.width());

    if (textRect.width() < textLayoutSize.width()
        || textRect.height() < textLayoutSize.height()) {
        QString elided;
        int start = 0;
        int end = text.indexOf(QChar::LineSeparator, start);
        if (end == -1) {
            elided += option.fontMetrics.elidedText(text, option.textElideMode, textRect.width());
        } else {
            while (end != -1) {
                elided += option.fontMetrics.elidedText(text.mid(start, end - start),
                                                        option.textElideMode, textRect.width());
                elided += QChar::LineSeparator;
                start = end + 1;
                end = text.indexOf(QChar::LineSeparator, start);
            }
            // let's add the last line (after the last QChar::LineSeparator)
            elided += option.fontMetrics.elidedText(text.mid(start),
                                                    option.textElideMode, textRect.width());
        }
        textLayout.setText(elided);
        textLayoutSize = doTextLayout(&textLayout, textRect.width());
    }

    const QSize layoutSize(textRect.width(), int(textLayoutSize.height()));
    const QRect layoutRect = QStyle::alignedRect(option.direction, option.displayAlignment,
                                                  layoutSize, textRect);
    // if we still overflow even after eliding the text, enable clipping
    if (!hasClipping() && (textRect.width() < textLayoutSize.width()
                           || textRect.height() < textLayoutSize.height())) {
        painter->save();
        painter->setClipRect(layoutRect);
        textLayout.draw(painter, layoutRect.topLeft(), format, layoutRect);
        painter->restore();
    } else {
        textLayout.draw(painter, layoutRect.topLeft(), format, layoutRect);
    }
}

} // namespace Utils
