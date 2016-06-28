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

#pragma once

#include "consoleitemmodel.h"
#include "console.h"

#include <QStyledItemDelegate>

QT_FORWARD_DECLARE_CLASS(QTextLayout)

namespace Debugger {
namespace Internal {

class ConsoleItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ConsoleItemDelegate(ConsoleItemModel *model, QObject *parent);

    void emitSizeHintChanged(const QModelIndex &index);
    QColor drawBackground(QPainter *painter, const QRect &rect, const QModelIndex &index,
                          bool selected) const;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const;

private:
    qreal layoutText(QTextLayout &tl, int width, bool *success = 0) const;

private:
    ConsoleItemModel *m_model;
    const QIcon m_logIcon;
    const QIcon m_warningIcon;
    const QIcon m_errorIcon;
    const QIcon m_expandIcon;
    const QIcon m_collapseIcon;
    const QIcon m_prompt;
    mutable int m_cachedHeight;
    mutable QFont m_cachedFont;
};

/*
  +----------------------------------------------------------------------------+
  | TYPEICONAREA  EXPANDABLEICONAREA  TEXTAREA               FILEAREA LINEAREA |
  +----------------------------------------------------------------------------+

 */
/*
  +----------------------------------------------------------------------------+
  | PROMPTAREA  EDITABLEAREA                                                   |
  +----------------------------------------------------------------------------+

 */
class ConsoleItemPositions
{
public:
    ConsoleItemPositions(ConsoleItemModel *model, const QRect &rect,
            const QFont &font, bool showTaskIconArea, bool showExpandableIconArea)
        : m_x(rect.x()),
          m_width(rect.width()),
          m_top(rect.top()),
          m_bottom(rect.bottom()),
          m_maxFileLength(0),
          m_maxLineLength(0),
          m_showTaskIconArea(showTaskIconArea),
          m_showExpandableIconArea(showExpandableIconArea)
    {
        m_fontHeight = QFontMetrics(font).height();
        m_maxFileLength = model->sizeOfFile(font);
        m_maxLineLength = model->sizeOfLineNumber(font);
    }

    int adjustedTop() const { return m_top + ITEM_PADDING; }
    int adjustedLeft() const { return m_x + ITEM_PADDING; }
    int adjustedRight() const { return m_width - ITEM_PADDING; }
    int adjustedBottom() const { return m_bottom; }
    int lineHeight() const { return m_fontHeight + 1; }
    int minimumHeight() const { return typeIconHeight() + 2 * ITEM_PADDING; }

    // PROMPTAREA is same as TYPEICONAREA
    int typeIconLeft() const { return adjustedLeft(); }
    int typeIconWidth() const { return TASK_ICON_SIZE; }
    int typeIconHeight() const { return TASK_ICON_SIZE; }
    int typeIconRight() const { return m_showTaskIconArea ? typeIconLeft() + typeIconWidth()
                                                          : adjustedLeft(); }
    QRect typeIcon() const { return QRect(typeIconLeft(), adjustedTop(), typeIconWidth(),
                                          typeIconHeight()); }

    int expandCollapseIconLeft() const { return typeIconRight() + ITEM_SPACING; }
    int expandCollapseIconWidth() const { return TASK_ICON_SIZE; }
    int expandCollapseIconHeight() const { return TASK_ICON_SIZE; }
    int expandCollapseIconRight() const { return m_showExpandableIconArea ?
                    expandCollapseIconLeft() + expandCollapseIconWidth() : typeIconRight(); }
    QRect expandCollapseIcon() const { return QRect(expandCollapseIconLeft(), adjustedTop(),
                                                    expandCollapseIconWidth(),
                                                    expandCollapseIconHeight()); }

    int textAreaLeft() const { return  expandCollapseIconRight() + ITEM_SPACING; }
    int textAreaWidth() const { return textAreaRight() - textAreaLeft(); }
    int textAreaRight() const { return fileAreaLeft() - ITEM_SPACING; }
    QRect textArea() const { return QRect(textAreaLeft(), adjustedTop(), textAreaWidth(),
                                          lineHeight()); }

    int fileAreaLeft() const { return fileAreaRight() - fileAreaWidth(); }
    int fileAreaWidth() const { return m_maxFileLength; }
    int fileAreaRight() const { return lineAreaLeft() - ITEM_SPACING; }
    QRect fileArea() const { return QRect(fileAreaLeft(), adjustedTop(), fileAreaWidth(),
                                          lineHeight()); }

    int lineAreaLeft() const { return lineAreaRight() - lineAreaWidth(); }
    int lineAreaWidth() const { return m_maxLineLength; }
    int lineAreaRight() const { return adjustedRight() - ITEM_SPACING; }
    QRect lineArea() const { return QRect(lineAreaLeft(), adjustedTop(), lineAreaWidth(),
                                          lineHeight()); }

private:
    int m_x;
    int m_width;
    int m_top;
    int m_bottom;
    int m_fontHeight;
    int m_maxFileLength;
    int m_maxLineLength;
    bool m_showTaskIconArea;
    bool m_showExpandableIconArea;

public:
    static const int TASK_ICON_SIZE = 16;
    static const int ITEM_PADDING = 8;
    static const int ITEM_SPACING = 4;
};

} // namespace Internal
} // namespace Debugger
