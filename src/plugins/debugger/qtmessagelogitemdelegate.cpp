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

#include "qtmessagelogitemdelegate.h"
#include "qtmessagelogeditor.h"

#include <QPainter>
#include <QTreeView>
#include <QScrollBar>

const char CONSOLE_LOG_BACKGROUND_COLOR[] = "#E8EEF2";
const char CONSOLE_WARNING_BACKGROUND_COLOR[] = "#F6F4EB";
const char CONSOLE_ERROR_BACKGROUND_COLOR[] = "#F6EBE7";
const char CONSOLE_EDITOR_BACKGROUND_COLOR[] = "#F7F7F7";

const char CONSOLE_LOG_BACKGROUND_SELECTED_COLOR[] = "#CDDEEA";
const char CONSOLE_WARNING_BACKGROUND_SELECTED_COLOR[] = "#F3EED1";
const char CONSOLE_ERROR_BACKGROUND_SELECTED_COLOR[] = "#F5D4CB";
const char CONSOLE_EDITOR_BACKGROUND_SELECTED_COLOR[] = "#DEDEDE";

const char CONSOLE_LOG_TEXT_COLOR[] = "#333333";
const char CONSOLE_WARNING_TEXT_COLOR[] = "#666666";
const char CONSOLE_ERROR_TEXT_COLOR[] = "#1D5B93";
const char CONSOLE_EDITOR_TEXT_COLOR[] = "#000000";

const char CONSOLE_BORDER_COLOR[] = "#C9C9C9";

const int ELLIPSIS_GRADIENT_WIDTH = 16;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// QtMessageLogItemDelegate
//
///////////////////////////////////////////////////////////////////////

QtMessageLogItemDelegate::QtMessageLogItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent),
    m_logIcon(QLatin1String(":/debugger/images/log.png")),
    m_warningIcon(QLatin1String(":/debugger/images/warning.png")),
    m_errorIcon(QLatin1String(":/debugger/images/error.png")),
    m_expandIcon(QLatin1String(":/debugger/images/expand.png")),
    m_collapseIcon(QLatin1String(":/debugger/images/collapse.png")),
    m_prompt(QLatin1String(":/debugger/images/prompt.png")),
    m_itemModel(0),
    m_cachedHeight(0)
{
}

void QtMessageLogItemDelegate::emitSizeHintChanged(const QModelIndex &index)
{
    emit sizeHintChanged(index);
}

QColor QtMessageLogItemDelegate::drawBackground(QPainter *painter, const QRect &rect,
                                         const QModelIndex &index,
                                         bool selected) const
{
    painter->save();
    QtMessageLogHandler::ItemType itemType = (QtMessageLogHandler::ItemType)index.data(
                QtMessageLogHandler::TypeRole).toInt();
    QColor backgroundColor;
    switch (itemType) {
    case QtMessageLogHandler::DebugType:
        backgroundColor = selected ? QColor(CONSOLE_LOG_BACKGROUND_SELECTED_COLOR) :
                                     QColor(CONSOLE_LOG_BACKGROUND_COLOR);
        break;
    case QtMessageLogHandler::WarningType:
        backgroundColor = selected ? QColor(CONSOLE_WARNING_BACKGROUND_SELECTED_COLOR) :
                                     QColor(CONSOLE_WARNING_BACKGROUND_COLOR);
        break;
    case QtMessageLogHandler::ErrorType:
        backgroundColor = selected ? QColor(CONSOLE_ERROR_BACKGROUND_SELECTED_COLOR) :
                                     QColor(CONSOLE_ERROR_BACKGROUND_COLOR);
        break;
    case QtMessageLogHandler::InputType:
    default:
        backgroundColor = selected ? QColor(CONSOLE_EDITOR_BACKGROUND_SELECTED_COLOR) :
                                     QColor(CONSOLE_EDITOR_BACKGROUND_COLOR);
        break;
    }
    if (!(index.flags() & Qt::ItemIsEditable))
        painter->setBrush(backgroundColor);
    painter->setPen(Qt::NoPen);
    painter->drawRect(rect);

    // Separator lines
    painter->setPen(QColor(CONSOLE_BORDER_COLOR));
    if (!(index.flags() & Qt::ItemIsEditable))
        painter->drawLine(0, rect.bottom(), rect.right(),
                      rect.bottom());
    painter->restore();
    return backgroundColor;
}

void QtMessageLogItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    painter->save();

    //Set Colors
    QColor textColor;
    QIcon taskIcon;
    QtMessageLogHandler::ItemType type = (QtMessageLogHandler::ItemType)index.data(
                QtMessageLogHandler::TypeRole).toInt();
    switch (type) {
    case QtMessageLogHandler::DebugType:
        textColor = QColor(CONSOLE_LOG_TEXT_COLOR);
        taskIcon = m_logIcon;
        break;
    case QtMessageLogHandler::WarningType:
        textColor = QColor(CONSOLE_WARNING_TEXT_COLOR);
        taskIcon = m_warningIcon;
        break;
    case QtMessageLogHandler::ErrorType:
        textColor = QColor(CONSOLE_ERROR_TEXT_COLOR);
        taskIcon = m_errorIcon;
        break;
    case QtMessageLogHandler::InputType:
        textColor = QColor(CONSOLE_EDITOR_TEXT_COLOR);
        taskIcon = m_prompt;
        break;
    default:
        textColor = QColor(CONSOLE_EDITOR_TEXT_COLOR);
        break;
    }

    //Paint background
    QColor backgroundColor = drawBackground(painter, opt.rect, index,
                   bool(opt.state & QStyle::State_Selected));

    //Calculate positions
    const QTreeView *view = qobject_cast<const QTreeView *>(opt.widget);
    int level = 0;
    QModelIndex idx(index);
    while (idx.parent() != QModelIndex()) {
        idx = idx.parent();
        level++;
    }
    int width = view->width() - level * view->indentation() -
            view->verticalScrollBar()->width();
    bool showTypeIcon = index.parent() == QModelIndex();
    bool showExpandableIcon = type == QtMessageLogHandler::UndefinedType;

    QRect rect(opt.rect.x(), opt.rect.top(), width, opt.rect.height());
    ConsoleItemPositions positions(rect, opt.font, showTypeIcon,
                        showExpandableIcon, m_itemModel);

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
        painter->drawText(positions.textArea(),
                          fm.elidedText(str, Qt::ElideRight,
                                        positions.textAreaWidth()));
    }
    //skip if area is editable
    if (showExpandableIcon) {
        // Paint ExpandableIconArea:
        QIcon expandCollapseIcon;
        if (index.model()->rowCount(index)) {
            if (view->isExpanded(index))
                expandCollapseIcon = m_collapseIcon;
            else
                expandCollapseIcon = m_expandIcon;
        }
        painter->drawPixmap(positions.expandCollapseIconLeft(),
                            positions.adjustedTop(),
                            expandCollapseIcon.pixmap(
                                positions.expandCollapseIconWidth(),
                                positions.expandCollapseIconHeight()));
    }

    if (showFileLineInfo) {
        //Check for file info
        QString file = index.data(QtMessageLogHandler::FileRole).toString();
        if (!file.isEmpty()) {
            QFontMetrics fm(option.font);
            // Paint FileArea
            const int pos = file.lastIndexOf(QLatin1Char('/'));
            if (pos != -1)
                file = file.mid(pos +1);
            const int realFileWidth = fm.width(file);
            painter->setClipRect(positions.fileArea());
            painter->drawText(positions.fileAreaLeft(),
                              positions.adjustedTop() + fm.ascent(), file);
            if (realFileWidth > positions.fileAreaWidth()) {
                // draw a gradient to mask the text
                int gradientStart = positions.fileAreaLeft() - 1;
                QLinearGradient lg(gradientStart +
                                   ELLIPSIS_GRADIENT_WIDTH, 0, gradientStart, 0);
                lg.setColorAt(0, Qt::transparent);
                lg.setColorAt(1, backgroundColor);
                painter->fillRect(gradientStart, positions.adjustedTop(),
                                  ELLIPSIS_GRADIENT_WIDTH, positions.lineHeight(),
                                  lg);
            }

            // Paint LineArea
            QString lineText  = index.data(QtMessageLogHandler::LineRole).toString();
            painter->setClipRect(positions.lineArea());
            const int realLineWidth = fm.width(lineText);
            painter->drawText(positions.lineAreaRight() - realLineWidth,
                              positions.adjustedTop() + fm.ascent(), lineText);
        }
    }
    painter->setClipRect(opt.rect);
    painter->restore();
}

QSize QtMessageLogItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    const QTreeView *view = qobject_cast<const QTreeView *>(opt.widget);
    int level = 0;
    QModelIndex idx(index);
    while (idx.parent() != QModelIndex()) {
        idx = idx.parent();
        level++;
    }
    int width = view->width() - level * view->indentation() -
            view->verticalScrollBar()->width();
    if (index.flags() & Qt::ItemIsEditable)
        return QSize(width, view->height() * 1/2);

    const bool selected = (view->selectionModel()->currentIndex() == index);
    if (!selected && option.font == m_cachedFont && m_cachedHeight > 0)
        return QSize(width, m_cachedHeight);

    QtMessageLogHandler::ItemType type = (QtMessageLogHandler::ItemType)index.data(
                QtMessageLogHandler::TypeRole).toInt();
    bool showTypeIcon = index.parent() == QModelIndex();
    bool showExpandableIcon = type == QtMessageLogHandler::UndefinedType;

    QRect rect(level * view->indentation(), 0, width, 0);
    ConsoleItemPositions positions(rect, opt.font,
                        showTypeIcon, showExpandableIcon, m_itemModel);

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

QWidget *QtMessageLogItemDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &/*option*/,
                                    const QModelIndex &index) const

{
    QtMessageLogEditor *editor = new QtMessageLogEditor(index, parent);
    connect(editor, SIGNAL(editingFinished()),
            this, SLOT(commitAndCloseEditor()));
    return editor;
}

void QtMessageLogItemDelegate::setEditorData(QWidget *editor,
                                 const QModelIndex &index) const
{
    QtMessageLogEditor *edtr = qobject_cast<QtMessageLogEditor *>(editor);
    edtr->insertPlainText(index.data(Qt::DisplayRole).toString());
}

void QtMessageLogItemDelegate::setModelData(QWidget *editor,
                                       QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    QtMessageLogEditor *edtr = qobject_cast<QtMessageLogEditor *>(editor);
    model->setData(index, edtr->getCurrentScript(), Qt::DisplayRole);
    model->setData(index, QtMessageLogHandler::InputType, QtMessageLogHandler::TypeRole);
}

void QtMessageLogItemDelegate::updateEditorGeometry(QWidget *editor,
                                               const QStyleOptionViewItem &option,
                                               const QModelIndex &/*index*/) const
{
    QStyleOptionViewItemV4 opt = option;
    editor->setGeometry(QRect(opt.rect.x(), opt.rect.top(),
                              opt.rect.width(), opt.rect.bottom()));
}

void QtMessageLogItemDelegate::currentChanged(const QModelIndex &current,
                                         const QModelIndex &previous)
{
    emit sizeHintChanged(current);
    emit sizeHintChanged(previous);
}

void QtMessageLogItemDelegate::commitAndCloseEditor()
{
    QtMessageLogEditor *editor = qobject_cast<QtMessageLogEditor *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

qreal QtMessageLogItemDelegate::layoutText(QTextLayout &tl, int width,
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

void QtMessageLogItemDelegate::setItemModel(QtMessageLogHandler *model)
{
    m_itemModel = model;
}

} //Internal
} //Debugger
