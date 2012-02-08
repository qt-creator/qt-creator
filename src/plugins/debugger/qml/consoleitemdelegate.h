/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CONSOLEITEMDELEGATE_H
#define CONSOLEITEMDELEGATE_H

#include "consoleitemmodel.h"

#include <QTextLayout>
#include <QStyledItemDelegate>

namespace Debugger {
namespace Internal {

class ConsoleBackend;
class ConsoleItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ConsoleItemDelegate(QObject *parent = 0);
    void emitSizeHintChanged(const QModelIndex &index);
    void setConsoleBackend(ConsoleBackend *consoleBackend);

    void drawBackground(QPainter *painter, const QRect &rect,
                      ConsoleItemModel::ItemType itemType, bool selected) const;

public slots:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const;

signals:
    void appendEditableRow();

private slots:
    void commitAndCloseEditor();

private:
    qreal layoutText(QTextLayout &tl, int width) const;

private:
    const QIcon m_logIcon;
    const QIcon m_warningIcon;
    const QIcon m_errorIcon;
    const QIcon m_expandIcon;
    const QIcon m_collapseIcon;
    const QIcon m_prompt;
    ConsoleBackend *m_consoleBackend;
};

/*
  +----------------------------------------------------------------------------+
  | TYPEICONAREA  EXPANDABLEICONAREA  TEXTAREA                                 |
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
    ConsoleItemPositions(const QRect &rect,
              const QFont &font = QFont(),
              bool showTaskIconArea = true,
              bool showExpandableIconArea = true)
        : m_x(rect.x()),
          m_width(rect.width()),
          m_top(rect.top()),
          m_bottom(rect.bottom()),
          m_showTaskIconArea(showTaskIconArea),
          m_showExpandableIconArea(showExpandableIconArea)
    {
        m_fontHeight = QFontMetrics(font).height();
    }

    int adjustedTop() const { return m_top + ITEM_PADDING; }
    int adjustedLeft() const { return m_x + ITEM_PADDING; }
    int adjustedRight() const { return m_width - ITEM_PADDING; }
    int adjustedBottom() const { return m_bottom; }
    int lineHeight() const { return m_fontHeight + 1; }
    int minimumHeight() const { return typeIconHeight() + 2 * ITEM_PADDING; }

    //PROMPTAREA is same as TYPEICONAREA
    int typeIconLeft() const { return adjustedLeft(); }
    int typeIconWidth() const { return TASK_ICON_SIZE; }
    int typeIconHeight() const { return TASK_ICON_SIZE; }
    int typeIconRight() const { return m_showTaskIconArea ?
                    typeIconLeft() + typeIconWidth() : adjustedLeft(); }
    QRect typeIcon() const { return
                QRect(typeIconLeft(), adjustedTop(),
                      typeIconWidth(), typeIconHeight()); }

    int expandCollapseIconLeft() const { return typeIconRight() +
                ITEM_SPACING; }
    int expandCollapseIconWidth() const { return TASK_ICON_SIZE; }
    int expandCollapseIconHeight() const { return TASK_ICON_SIZE; }
    int expandCollapseIconRight() const { return m_showExpandableIconArea ?
                    expandCollapseIconLeft() + expandCollapseIconWidth() :
                    typeIconRight(); }
    QRect expandCollapseIcon() const { return
                QRect(expandCollapseIconLeft(), adjustedTop(),
                      expandCollapseIconWidth(), expandCollapseIconHeight()); }

    int textAreaLeft() const { return  expandCollapseIconRight() + ITEM_SPACING; }
    int textAreaWidth() const { return textAreaRight() - textAreaLeft(); }
    int textAreaRight() const { return adjustedRight() - ITEM_SPACING; }
    QRect textArea() const { return
                QRect(textAreaLeft(), adjustedTop(), textAreaWidth(), lineHeight()); }

private:
    int m_x;
    int m_width;
    int m_top;
    int m_bottom;
    int m_fontHeight;
    bool m_showTaskIconArea;
    bool m_showExpandableIconArea;

public:
    static const int TASK_ICON_SIZE = 16;
    static const int ITEM_PADDING = 2;
    static const int ITEM_SPACING = 2 * ITEM_PADDING;

};
} //Internal
} //Debugger

#endif // CONSOLEITEMDELEGATE_H
