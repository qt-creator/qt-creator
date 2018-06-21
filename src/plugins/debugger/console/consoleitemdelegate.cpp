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

#include "consoleitemdelegate.h"
#include "consoleedit.h"

#include <coreplugin/coreconstants.h>

#include <utils/utilsicons.h>
#include <utils/theme/theme.h>

#include <QPainter>
#include <QTreeView>
#include <QScrollBar>
#include <QTextLayout>
#include <QUrl>

const int ELLIPSIS_GRADIENT_WIDTH = 16;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// ConsoleItemDelegate
//
///////////////////////////////////////////////////////////////////////

ConsoleItemDelegate::ConsoleItemDelegate(ConsoleItemModel *model, QObject *parent) :
    QStyledItemDelegate(parent),
    m_model(model),
    m_logIcon(Utils::Icons::INFO.icon()),
    m_warningIcon(Utils::Icons::WARNING.icon()),
    m_errorIcon(Utils::Icons::CRITICAL.icon()),
    m_expandIcon(Utils::Icons::EXPAND.icon()),
    m_collapseIcon(Utils::Icons::COLLAPSE.icon()),
    m_prompt(Utils::Icon({{QLatin1String(":/utils/images/next.png"),
                          Utils::Theme::TextColorNormal}}, Utils::Icon::Tint).icon()),
    m_cachedHeight(0)
{
}

void ConsoleItemDelegate::emitSizeHintChanged(const QModelIndex &index)
{
    emit sizeHintChanged(index);
}

QColor ConsoleItemDelegate::drawBackground(QPainter *painter, const QRect &rect,
                                              const QModelIndex &index,
                                              bool selected) const
{
    const Utils::Theme *theme = Utils::creatorTheme();
    painter->save();
    QColor backgroundColor = theme->color(selected
                                          ? Utils::Theme::BackgroundColorSelected
                                          : Utils::Theme::BackgroundColorNormal);
    if (!(index.flags() & Qt::ItemIsEditable))
        painter->setBrush(backgroundColor);
    painter->setPen(Qt::NoPen);
    painter->drawRect(rect);
    painter->restore();
    return backgroundColor;
}

void ConsoleItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    const Utils::Theme *theme = Utils::creatorTheme();
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();

    // Set Colors
    QColor textColor;
    QIcon taskIcon;
    ConsoleItem::ItemType type = (ConsoleItem::ItemType)index.data(
                ConsoleItem::TypeRole).toInt();
    switch (type) {
    case ConsoleItem::DebugType:
        textColor = theme->color(Utils::Theme::OutputPanes_NormalMessageTextColor);
        taskIcon = m_logIcon;
        break;
    case ConsoleItem::WarningType:
        textColor = theme->color(Utils::Theme::OutputPanes_WarningMessageTextColor);
        taskIcon = m_warningIcon;
        break;
    case ConsoleItem::ErrorType:
        textColor = theme->color(Utils::Theme::OutputPanes_ErrorMessageTextColor);
        taskIcon = m_errorIcon;
        break;
    case ConsoleItem::InputType:
        textColor = theme->color(Utils::Theme::TextColorNormal);
        taskIcon = m_prompt;
        break;
    default:
        textColor = theme->color(Utils::Theme::TextColorNormal);
        break;
    }

    // Paint background
    QColor backgroundColor = drawBackground(painter, opt.rect, index,
                                            bool(opt.state & QStyle::State_Selected));

    // Calculate positions
    const QTreeView *view = qobject_cast<const QTreeView *>(opt.widget);
    int level = 0;
    QModelIndex idx(index);
    while (idx.parent() != QModelIndex()) {
        idx = idx.parent();
        level++;
    }
    int width = view->width() - level * view->indentation() - view->verticalScrollBar()->width();
    bool showTypeIcon = index.parent() == QModelIndex();
    bool showExpandableIcon = type == ConsoleItem::DefaultType;

    QRect rect(opt.rect.x(), opt.rect.top(), width, opt.rect.height());
    ConsoleItemPositions positions(m_model, rect, opt.font, showTypeIcon, showExpandableIcon);

    // Paint TaskIconArea:
    if (showTypeIcon)
        painter->drawPixmap(positions.adjustedLeft(), positions.adjustedTop(),
                            taskIcon.pixmap(positions.typeIconWidth(),
                                            positions.typeIconHeight()));

    // Set Text Color
    painter->setPen(textColor);
    // Paint TextArea:
    // Layout the description
    QString str = index.data(Qt::DisplayRole).toString();
    bool showFileLineInfo = true;
    // show complete text if selected
    if (view->selectionModel()->currentIndex() == index) {
        QTextLayout tl(str, opt.font);
        layoutText(tl, positions.textAreaWidth(), &showFileLineInfo);
        tl.draw(painter, QPoint(positions.textAreaLeft(), positions.adjustedTop()));
    } else {
        QFontMetrics fm(opt.font);
        painter->drawText(positions.textArea(), fm.elidedText(str, Qt::ElideRight,
                                                              positions.textAreaWidth()));
    }
    // skip if area is editable
    if (showExpandableIcon) {
        // Paint ExpandableIconArea:
        QIcon expandCollapseIcon;
        if (index.model()->rowCount(index) || index.model()->canFetchMore(index)) {
            if (view->isExpanded(index))
                expandCollapseIcon = m_collapseIcon;
            else
                expandCollapseIcon = m_expandIcon;
        }
        painter->drawPixmap(positions.expandCollapseIconLeft(), positions.adjustedTop(),
                            expandCollapseIcon.pixmap(positions.expandCollapseIconWidth(),
                                                      positions.expandCollapseIconHeight()));
    }

    if (showFileLineInfo) {
        // Check for file info
        QString file = index.data(ConsoleItem::FileRole).toString();
        const QUrl fileUrl = QUrl(file);
        if (fileUrl.isLocalFile())
            file = fileUrl.toLocalFile();
        if (!file.isEmpty()) {
            QFontMetrics fm(option.font);
            // Paint FileArea
            const int pos = file.lastIndexOf(QLatin1Char('/'));
            if (pos != -1)
                file = file.mid(pos +1);
            const int realFileWidth = fm.width(file);
            painter->setClipRect(positions.fileArea());
            painter->drawText(positions.fileAreaLeft(), positions.adjustedTop() + fm.ascent(),
                              file);
            if (realFileWidth > positions.fileAreaWidth()) {
                // draw a gradient to mask the text
                int gradientStart = positions.fileAreaLeft() - 1;
                QLinearGradient lg(gradientStart + ELLIPSIS_GRADIENT_WIDTH, 0, gradientStart, 0);
                lg.setColorAt(0, Qt::transparent);
                lg.setColorAt(1, backgroundColor);
                painter->fillRect(gradientStart, positions.adjustedTop(),
                                  ELLIPSIS_GRADIENT_WIDTH, positions.lineHeight(), lg);
            }

            // Paint LineArea
            QString lineText  = index.data(ConsoleItem::LineRole).toString();
            painter->setClipRect(positions.lineArea());
            const int realLineWidth = fm.width(lineText);
            painter->drawText(positions.lineAreaRight() - realLineWidth,
                              positions.adjustedTop() + fm.ascent(), lineText);
        }
    }
    painter->setClipRect(opt.rect);
    painter->restore();
}

QSize ConsoleItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QTreeView *view = qobject_cast<const QTreeView *>(opt.widget);
    int level = 0;
    QModelIndex idx(index);
    while (idx.parent() != QModelIndex()) {
        idx = idx.parent();
        level++;
    }
    int width = view->width() - level * view->indentation() - view->verticalScrollBar()->width();

    const bool selected = (view->selectionModel()->currentIndex() == index);
    if (!selected && option.font == m_cachedFont && m_cachedHeight > 0)
        return QSize(width, m_cachedHeight);

    ConsoleItem::ItemType type = (ConsoleItem::ItemType)index.data(
                ConsoleItem::TypeRole).toInt();
    bool showTypeIcon = index.parent() == QModelIndex();
    bool showExpandableIcon = type == ConsoleItem::DefaultType;

    QRect rect(level * view->indentation(), 0, width, 0);
    ConsoleItemPositions positions(m_model, rect, opt.font, showTypeIcon, showExpandableIcon);

    QFontMetrics fm(option.font);
    qreal height = fm.height();

    if (selected) {
        QString str = index.data(Qt::DisplayRole).toString();

        QTextLayout tl(str, option.font);
        height = layoutText(tl, positions.textAreaWidth());
    }

    height += 2 * ConsoleItemPositions::ITEM_PADDING;

    if (height < positions.minimumHeight())
        height = positions.minimumHeight();

    if (!selected) {
        m_cachedHeight = height;
        m_cachedFont = option.font;
    }

    return QSize(width, height);
}

QWidget *ConsoleItemDelegate::createEditor(QWidget *parent,
                                              const QStyleOptionViewItem &/*option*/,
                                              const QModelIndex &index) const

{
    ConsoleEdit *editor = new ConsoleEdit(index, parent);
    // Make the background transparent so that the prompt shines through
    editor->setStyleSheet(QLatin1String("QTextEdit {"
                                        "margin-left: 24px;"
                                        "margin-top: 4px;"
                                        "background-color: transparent;"
                                        "}"));
    connect(editor, &ConsoleEdit::editingFinished, this, [this, editor] {
        auto delegate = const_cast<ConsoleItemDelegate*>(this);
        emit delegate->commitData(editor);
        emit delegate->closeEditor(editor);
    });
    return editor;
}

void ConsoleItemDelegate::setEditorData(QWidget *editor,
                                           const QModelIndex &index) const
{
    ConsoleEdit *edtr = qobject_cast<ConsoleEdit *>(editor);
    edtr->insertPlainText(index.data(ConsoleItem::ExpressionRole).toString());
}

void ConsoleItemDelegate::setModelData(QWidget *editor,
                                          QAbstractItemModel *model,
                                          const QModelIndex &index) const
{
    ConsoleEdit *edtr = qobject_cast<ConsoleEdit *>(editor);
    model->setData(index, edtr->getCurrentScript(), ConsoleItem::ExpressionRole);
    model->setData(index, ConsoleItem::InputType, ConsoleItem::TypeRole);
}

void ConsoleItemDelegate::updateEditorGeometry(QWidget *editor,
                                                  const QStyleOptionViewItem &option,
                                                  const QModelIndex &/*index*/) const
{
    editor->setGeometry(QRect(option.rect.x(), option.rect.top(), option.rect.width(), option.rect.bottom()));
}

void ConsoleItemDelegate::currentChanged(const QModelIndex &current,
                                            const QModelIndex &previous)
{
    emit sizeHintChanged(current);
    emit sizeHintChanged(previous);
}

qreal ConsoleItemDelegate::layoutText(QTextLayout &tl, int width,
                                         bool *showFileLineInfo) const
{
    qreal height = 0;
    tl.beginLayout();
    while (true) {
        QTextLine line = tl.createLine();

        if (!line.isValid())
            break;
        line.setLeadingIncluded(true);
        line.setLineWidth(width);
        if (width < line.naturalTextWidth() && showFileLineInfo)
            *showFileLineInfo = false;
        line.setPosition(QPoint(0, height));
        height += line.height();
    }
    tl.endLayout();
    return height;
}

} // Internal
} // Debugger
