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

#include "consoletreeview.h"
#include "consoleitemdelegate.h"
#include "consoleitemmodel.h"
#include "debuggerinternalconstants.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QProxyStyle>
#include <QtGui/QPainter>

namespace Debugger {
namespace Internal {

class ConsoleTreeViewStyle : public QProxyStyle
{
public:
    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = 0) const
    {
        if (element != QStyle::PE_PanelItemViewRow)
            QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    int styleHint(StyleHint hint, const QStyleOption *option = 0,
                  const QWidget *widget = 0,
                  QStyleHintReturn *returnData = 0) const {
        if (hint == SH_ItemView_ShowDecorationSelected)
            return 0;
        else
            return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

///////////////////////////////////////////////////////////////////////
//
// ConsoleTreeView
//
///////////////////////////////////////////////////////////////////////

ConsoleTreeView::ConsoleTreeView(QWidget *parent) :
    QTreeView(parent)
{
    setFrameStyle(QFrame::NoFrame);
    setHeaderHidden(true);
    setRootIsDecorated(false);
    setEditTriggers(QAbstractItemView::AllEditTriggers);
    setStyleSheet(QLatin1String("QTreeView::branch:has-siblings:!adjoins-item {"
                                "border-image: none;"
                                "image: none; }"
                                "QTreeView::branch:has-siblings:adjoins-item {"
                                "border-image: none;"
                                "image: none; }"
                                "QTreeView::branch:!has-children:!has-siblings:adjoins-item {"
                                "border-image: none;"
                                "image: none; }"
                                "QTreeView::branch:has-children:!has-siblings:closed,"
                                "QTreeView::branch:closed:has-children:has-siblings {"
                                "border-image: none;"
                                "image: none; }"
                                "QTreeView::branch:open:has-children:!has-siblings,"
                                "QTreeView::branch:open:has-children:has-siblings  {"
                                "border-image: none;"
                                "image: none; }"));
    ConsoleTreeViewStyle *style = new ConsoleTreeViewStyle;
    setStyle(style);
    style->setParent(this);
}

void ConsoleTreeView::setItemDelegate(QAbstractItemDelegate *delegate)
{
    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            delegate, SLOT(currentChanged(QModelIndex,QModelIndex)));
    QTreeView::setItemDelegate(delegate);
}

void ConsoleTreeView::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    if (index.isValid()) {
        ConsoleItemModel::ItemType type =
                (ConsoleItemModel::ItemType)index.data(
                    ConsoleItemModel::TypeRole).toInt();
        bool showTypeIcon = index.parent() == QModelIndex();
        bool showExpandableIcon = type != ConsoleItemModel::InputType &&
                type != ConsoleItemModel::UndefinedType;
        ConsoleItemPositions positions(visualRect(index), viewOptions().font,
                                       showTypeIcon, showExpandableIcon);

        if (positions.expandCollapseIcon().contains(pos)) {
            if (isExpanded(index))
                setExpanded(index, false);
            else
                setExpanded(index, true);
        } else {
            QTreeView::mousePressEvent(event);
        }
    } else {
        selectionModel()->setCurrentIndex(model()->index(
                                              model()->rowCount() - 1, 0),
                                 QItemSelectionModel::ClearAndSelect);
    }
}

void ConsoleTreeView::resizeEvent(QResizeEvent *e)
{
    static_cast<ConsoleItemDelegate *>(itemDelegate())->emitSizeHintChanged(
                selectionModel()->currentIndex());
    QTreeView::resizeEvent(e);
}

void ConsoleTreeView::drawBranches(QPainter *painter, const QRect &rect,
                             const QModelIndex &index) const
{
    ConsoleItemModel::ItemType type =
            (ConsoleItemModel::ItemType)index.data(
                ConsoleItemModel::TypeRole).toInt();
    static_cast<ConsoleItemDelegate *>(itemDelegate())->drawBackground(
                painter, rect, type, true);
    QTreeView::drawBranches(painter, rect, index);
}

} //Internal
} //Debugger
