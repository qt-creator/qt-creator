// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "searchresulttreeitemdelegate.h"
#include "searchresulttreeitemroles.h"

#include <QPainter>
#include <QApplication>

#include <QModelIndex>
#include <QDebug>

using namespace Core::Internal;

SearchResultTreeItemDelegate::SearchResultTreeItemDelegate(int tabWidth, QObject *parent)
    : QItemDelegate(parent)
{
    setTabWidth(tabWidth);
}

const int lineNumberAreaHorizontalPadding = 4;
const int minimumLineNumberDigits = 6;

static std::pair<int, QString> lineNumberInfo(const QStyleOptionViewItem &option,
                                              const QModelIndex &index)
{
    const int lineNumber = index.data(ItemDataRoles::ResultBeginLineNumberRole).toInt();
    if (lineNumber < 1)
        return {0, {}};
    const QString lineNumberText = QString::number(lineNumber);
    const int lineNumberDigits = qMax(minimumLineNumberDigits, lineNumberText.size());
    const int fontWidth = option.fontMetrics.horizontalAdvance(QString(lineNumberDigits, QLatin1Char('0')));
    const QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    return {lineNumberAreaHorizontalPadding + fontWidth + lineNumberAreaHorizontalPadding
                + style->pixelMetric(QStyle::PM_FocusFrameHMargin),
            lineNumberText};
}

// Aligns text by appending spaces
static QString align(QString text)
{
    constexpr int minimumTextSize = 80;
    constexpr int textSizeIncrement = 20;

    int textSize = ((text.size() / textSizeIncrement) + 1) * textSizeIncrement;
    textSize = std::max(minimumTextSize, textSize);
    text.resize(textSize, ' ');
    return text;
}

static QPair<QString, QString> itemText(const QModelIndex &index)
{
    QString text = index.data(Qt::DisplayRole).toString();
    // show number of subresults in displayString
    if (index.model()->hasChildren(index))
        text += " (" + QString::number(index.model()->rowCount(index)) + ')';

    const auto functionName = index.data(ItemDataRoles::ContainingFunctionNameRole).toString();
    if (!functionName.isEmpty())
        return {align(std::move(text)), "[in " + functionName + "]"};
    return {text, {}};
}

LayoutInfo SearchResultTreeItemDelegate::getLayoutInfo(const QStyleOptionViewItem &option,
                                                       const QModelIndex &index) const
{
    static const int iconSize = 16;

    LayoutInfo info;
    info.option = setOptions(index, option);

    // check mark
    const bool checkable = (index.model()->flags(index) & Qt::ItemIsUserCheckable);
    info.checkState = Qt::Unchecked;
    if (checkable) {
        QVariant checkStateData = index.data(Qt::CheckStateRole);
        info.checkState = static_cast<Qt::CheckState>(checkStateData.toInt());
        info.checkRect = doCheck(info.option, info.option.rect, checkStateData);
    }

    // icon
    info.icon = index.data(ItemDataRoles::ResultIconRole).value<QIcon>();
    if (!info.icon.isNull()) {
        const QSize size = info.icon.actualSize(QSize(iconSize, iconSize));
        info.pixmapRect = QRect(0, 0, size.width(), size.height());
    }

    // text
    info.textRect = info.option.rect.adjusted(0,
                                              0,
                                              info.checkRect.width() + info.pixmapRect.width(),
                                              0);

    // do basic layout
    doLayout(info.option, &info.checkRect, &info.pixmapRect, &info.textRect, false);

    // adapt for line numbers
    const int lineNumberWidth = lineNumberInfo(info.option, index).first;
    info.lineNumberRect = info.textRect;
    info.lineNumberRect.setWidth(lineNumberWidth);
    info.textRect.adjust(lineNumberWidth, 0, 0, 0);
    return info;
}

void SearchResultTreeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    const LayoutInfo info = getLayoutInfo(option, index);

    painter->setFont(info.option.font);

    QItemDelegate::drawBackground(painter, info.option, index);

    // ---- draw the items
    // icon
    if (!info.icon.isNull())
        info.icon.paint(painter, info.pixmapRect, info.option.decorationAlignment);

    // line numbers
    drawLineNumber(painter, info.option, info.lineNumberRect, index);

    // text and focus/selection
    drawText(painter, info.option, info.textRect, index);
    QItemDelegate::drawFocus(painter, info.option, info.option.rect);

    // check mark
    if (info.checkRect.isValid())
        QItemDelegate::drawCheck(painter, info.option, info.checkRect, info.checkState);

    painter->restore();
}

void SearchResultTreeItemDelegate::setTabWidth(int width)
{
    m_tabString = QString(width, QLatin1Char(' '));
}

QSize SearchResultTreeItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                             const QModelIndex &index) const
{
    const LayoutInfo info = getLayoutInfo(option, index);
    const int height = index.data(Qt::SizeHintRole).value<QSize>().height();
    // get text width, see QItemDelegatePrivate::displayRect
    auto texts = itemText(index);
    const QString text = texts.first.replace('\t', m_tabString)
                         + texts.second.replace('\t', m_tabString);
    const QRect textMaxRect(0, 0, INT_MAX / 256, height);
    const QRect textLayoutRect = textRectangle(nullptr, textMaxRect, info.option.font, text);
    const QRect textRect(info.textRect.x(), info.textRect.y(), textLayoutRect.width(), height);
    const QRect layoutRect = info.checkRect | info.pixmapRect | info.lineNumberRect | textRect;
    return QSize(layoutRect.x(), layoutRect.y()) + layoutRect.size();
}

// returns the width of the line number area
int SearchResultTreeItemDelegate::drawLineNumber(QPainter *painter, const QStyleOptionViewItem &option,
                                                 const QRect &rect,
                                                 const QModelIndex &index) const
{
    const bool isSelected = option.state & QStyle::State_Selected;
    const std::pair<int, QString> numberInfo = lineNumberInfo(option, index);
    if (numberInfo.first == 0)
        return 0;
    QRect lineNumberAreaRect(rect);
    lineNumberAreaRect.setWidth(numberInfo.first);

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
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, nullptr) + 1;

    const QRect rowRect = lineNumberAreaRect.adjusted(-textMargin, 0, textMargin-lineNumberAreaHorizontalPadding, 0);
    QItemDelegate::drawDisplay(painter, opt, rowRect, numberInfo.second);

    return numberInfo.first;
}

void SearchResultTreeItemDelegate::drawText(QPainter *painter,
                                            const QStyleOptionViewItem &option,
                                            const QRect &rect,
                                            const QModelIndex &index) const
{
    const auto texts = itemText(index);
    const QString text = texts.first;

    const int searchTermStart = index.model()->data(index, ItemDataRoles::ResultBeginColumnNumberRole).toInt();
    int searchTermLength = index.model()->data(index, ItemDataRoles::SearchTermLengthRole).toInt();
    if (searchTermStart < 0 || searchTermStart >= text.length() || searchTermLength < 1) {
        QItemDelegate::drawDisplay(painter,
                                   option,
                                   rect,
                                   QString(text).replace(QLatin1Char('\t'), m_tabString));
        return;
    }

    // clip searchTermLength to end of line
    searchTermLength = qMin(searchTermLength, text.length() - searchTermStart);
    const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    const QString textBefore = text.left(searchTermStart).replace(QLatin1Char('\t'), m_tabString);
    const QString textHighlight = text.mid(searchTermStart, searchTermLength).replace(QLatin1Char('\t'), m_tabString);
    const QString textAfter = text.mid(searchTermStart + searchTermLength).replace(QLatin1Char('\t'), m_tabString);
    int searchTermStartPixels = option.fontMetrics.horizontalAdvance(textBefore);
    int searchTermLengthPixels = option.fontMetrics.horizontalAdvance(textHighlight);
    int textAfterLengthPixels = option.fontMetrics.horizontalAdvance(textAfter);
    int containingFunctionLengthPixels = option.fontMetrics.horizontalAdvance(texts.second);

    // rects
    QRect beforeHighlightRect(rect);
    beforeHighlightRect.setRight(beforeHighlightRect.left() + searchTermStartPixels);

    QRect resultHighlightRect(rect);
    resultHighlightRect.setLeft(beforeHighlightRect.right());
    resultHighlightRect.setRight(resultHighlightRect.left() + searchTermLengthPixels);

    QRect afterHighlightRect(rect);
    afterHighlightRect.setLeft(resultHighlightRect.right());
    afterHighlightRect.setRight(afterHighlightRect.left() + textAfterLengthPixels);

    QRect containingFunctionRect(rect);
    containingFunctionRect.setLeft(afterHighlightRect.right());
    containingFunctionRect.setRight(afterHighlightRect.right() + containingFunctionLengthPixels);

    // paint all highlight backgrounds
    // qitemdelegate has problems with painting background when highlighted
    // (highlighted background at wrong position because text is offset with textMargin)
    // so we duplicate a lot here, see qitemdelegate for reference
    bool isSelected = option.state & QStyle::State_Selected;
    QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                              ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    QStyleOptionViewItem baseOption = option;
    baseOption.state &= ~QStyle::State_Selected;
    if (isSelected) {
        painter->fillRect(beforeHighlightRect.adjusted(textMargin, 0, textMargin, 0),
                          option.palette.brush(cg, QPalette::Highlight));
        painter->fillRect(afterHighlightRect.adjusted(textMargin, 0, textMargin, 0),
                          option.palette.brush(cg, QPalette::Highlight));
    }
    const QColor highlightBackground =
            index.model()->data(index, ItemDataRoles::ResultHighlightBackgroundColor).value<QColor>();
    painter->fillRect(resultHighlightRect.adjusted(textMargin, 0, textMargin - 1, 0),
                      QBrush(highlightBackground));

    // Background of containing function
    const QColor funcHighlightBackground
        = index.model()->data(index, ItemDataRoles::FunctionHighlightBackgroundColor).value<QColor>();
    painter->fillRect(containingFunctionRect.adjusted(textMargin, 0, textMargin - 1, 0),
                      QBrush(funcHighlightBackground));

    // Text before the highlighting
    QStyleOptionViewItem noHighlightOpt = baseOption;
    noHighlightOpt.rect = beforeHighlightRect;
    noHighlightOpt.textElideMode = Qt::ElideNone;
    if (isSelected)
        noHighlightOpt.palette.setColor(QPalette::Text, noHighlightOpt.palette.color(cg, QPalette::HighlightedText));
    QItemDelegate::drawDisplay(painter, noHighlightOpt, beforeHighlightRect, textBefore);

    // Highlight text
    QStyleOptionViewItem highlightOpt = noHighlightOpt;
    const QColor highlightForeground
        = index.model()->data(index, ItemDataRoles::ResultHighlightForegroundColor).value<QColor>();
    highlightOpt.palette.setColor(QPalette::Text, highlightForeground);
    QItemDelegate::drawDisplay(painter, highlightOpt, resultHighlightRect, textHighlight);

    // Text after the Highlight
    noHighlightOpt.rect = afterHighlightRect;
    QItemDelegate::drawDisplay(painter, noHighlightOpt, afterHighlightRect, textAfter);

    // Containing function
    const QColor funcHighlightForeground
        = index.model()->data(index, ItemDataRoles::FunctionHighlightForegroundColor).value<QColor>();
    highlightOpt.palette.setColor(QPalette::Text, funcHighlightForeground);
    highlightOpt.rect = containingFunctionRect;
    QItemDelegate::drawDisplay(painter, highlightOpt, containingFunctionRect, texts.second);
}
