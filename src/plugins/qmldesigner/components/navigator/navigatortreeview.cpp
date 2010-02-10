/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "navigatortreeview.h"

#include "navigatorview.h"
#include "navigatortreemodel.h"
#include "navigatorwidget.h"

#include <nodeproperty.h>
#include "metainfo.h"

#define _separator_line_color_ "#757575"

namespace QmlDesigner {

QSize IconCheckboxItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(15,21);
}

void IconCheckboxItemDelegate::paint(QPainter *painter,
                                     const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());
    bool isChecked= (m_TreeModel->itemFromIndex(index)->checkState() == Qt::Checked);

    if (m_TreeModel->isNodeInvisible( index ))
        painter->setOpacity(0.5);

    if (isChecked)
        painter->drawPixmap(option.rect.x()+2,option.rect.y()+1,onPix);
    else
        painter->drawPixmap(option.rect.x()+2,option.rect.y()+1,offPix);

    painter->restore();
}


void IdItemDelegate::paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());

    if (m_TreeModel->isNodeInvisible( index ))
        painter->setOpacity(0.5);

    ModelNode node = m_TreeModel->nodeForIndex(index);

    QIcon icon=node.metaInfo().icon();
    if (icon.isNull())
    {
        // if node has no own icon, search for it in the itemlibrary
        QList <ItemLibraryInfo> InfoList = node.metaInfo().metaInfo().itemLibraryRepresentations(node.metaInfo());
        foreach (const ItemLibraryInfo &entry, InfoList)
        {
            if (entry.typeName()==node.metaInfo().typeName()) {
                icon = entry.icon();
                break;
            }
        }

        // if the library was also empty, use the default icon
        if (icon.isNull()) icon = QIcon(":/ItemLibrary/images/default-icon.png");
    }

    QPixmap pixmap = icon.pixmap(option.rect.width(),option.rect.height()-4);
    painter->drawPixmap(option.rect.x()+5,option.rect.y()+2,pixmap);

    QString myString = node.id();
    if (myString.isEmpty())
        myString = node.simplifiedTypeName();

    // Check text length does not exceed available space
    int extraSpace=12+pixmap.width();
    QFontMetrics metric(painter->fontMetrics());
    if (painter->fontMetrics().boundingRect(myString).width() > option.rect.width()-extraSpace)
    {
        QString origString(myString);
        int cutpoint=origString.length()/2;
        while (painter->fontMetrics().boundingRect(myString).width() > option.rect.width()-extraSpace)
        {
            cutpoint--;
            myString = origString.left(cutpoint)+QLatin1String("...")+origString.right(cutpoint);
        }
    }

    painter->drawText(option.rect.bottomLeft()+QPoint(8+pixmap.width(),-4),myString);

    painter->restore();
}

void NavigatorTreeView::drawRow(QPainter *painter, const QStyleOptionViewItem &options, const QModelIndex &index) const
{
    painter->save();

    QTreeView::drawRow(painter,options,index);
    painter->setPen(QColor(_separator_line_color_));
    painter->drawLine(options.rect.bottomLeft(),options.rect.bottomRight());

    painter->restore();
}

}
