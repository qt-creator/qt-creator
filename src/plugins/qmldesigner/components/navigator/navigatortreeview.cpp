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

#include "navigatortreeview.h"

#include "navigatorview.h"
#include "navigatortreemodel.h"
#include "navigatorwidget.h"
#include "qproxystyle.h"

#include <nodeproperty.h>
#include "metainfo.h"
#include <QLineEdit>

namespace QmlDesigner {

static void drawSelectionBackground(QPainter *painter, const QStyleOption &option)
{
    painter->save();
    QLinearGradient gradient;
    QColor highlight = option.palette.highlight().color();
    gradient.setColorAt(0, highlight.lighter(130));
    gradient.setColorAt(1, highlight.darker(130));
    gradient.setStart(option.rect.topLeft());
    gradient.setFinalStop(option.rect.bottomLeft());
    painter->fillRect(option.rect, gradient);
    painter->setPen(highlight.lighter());
    painter->drawLine(option.rect.topLeft(),option.rect.topRight());
    painter->setPen(highlight.darker());
    painter->drawLine(option.rect.bottomLeft(),option.rect.bottomRight());
    painter->restore();
}

// This style basically allows us to span the entire row
// including the arrow indicators which would otherwise not be
// drawn by the delegate
class TreeViewStyle : public QProxyStyle
{
public:
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = 0) const
    {
        if (element == QStyle::PE_PanelItemViewRow) {
            if (option->state & QStyle::State_Selected) {
                drawSelectionBackground(painter, *option);
            } else {
                painter->save();
                painter->setPen(QColor(255, 255, 255, 15));
                painter->drawLine(option->rect.topLeft(), option->rect.topRight());
                painter->setPen(QColor(0, 0, 0, 25));
                painter->drawLine(option->rect.bottomLeft(),option->rect.bottomRight());
                painter->restore();
            }
        } else if (element == PE_IndicatorItemViewItemDrop) {
            painter->save();
            QRect rect = option->rect;
            rect.setLeft(0);
            rect.setWidth(widget->rect().width());
            QColor highlight = option->palette.text().color();
            highlight.setAlphaF(0.7);
            painter->setPen(QPen(highlight.lighter(), 1));
            if (option->rect.height() == 0) {
                if (option->rect.top()>0)
                    painter->drawLine(rect.topLeft(), rect.topRight());
            }
            else {
                highlight.setAlphaF(0.2);
                painter->setBrush(highlight);
                painter->drawRect(rect.adjusted(0, 0, -1, -1));
            }
            painter->restore();
        } else {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
        }
    }

    int styleHint(StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0, QStyleHintReturn *returnData = 0) const {
        if (hint == SH_ItemView_ShowDecorationSelected)
            return 0;
        else
            return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

NavigatorTreeView::NavigatorTreeView(QWidget *parent)
    : QTreeView(parent)
{
    TreeViewStyle *style = new TreeViewStyle;
    setStyle(style);
    style->setParent(this);
};

QSize IconCheckboxItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(15,26);
}

void IconCheckboxItemDelegate::paint(QPainter *painter,
                                     const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    if (option.state & QStyle::State_Selected)
        drawSelectionBackground(painter, option);

    if (!m_TreeModel->nodeForIndex(index).isRootNode()) {

        bool isChecked= (m_TreeModel->itemFromIndex(index)->checkState() == Qt::Checked);

        if (m_TreeModel->isNodeInvisible( index ))
            painter->setOpacity(0.5);

        if (isChecked)
            painter->drawPixmap(option.rect.x()+2,option.rect.y()+5,onPix);
        else
            painter->drawPixmap(option.rect.x()+2,option.rect.y()+5,offPix);
    }
    painter->restore();
}

void IdItemDelegate::paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (option.state & QStyle::State_Selected)
        drawSelectionBackground(painter, option);

    painter->save();

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

    QPixmap pixmap = icon.pixmap(22,22);
    painter->drawPixmap(option.rect.x(),option.rect.y()+2,pixmap);

    QString myString = node.id();
    if (myString.isEmpty())
        myString = node.simplifiedTypeName();

    // Check text length does not exceed available space
    int extraSpace=12+pixmap.width();
    QFontMetrics fm(option.font);
    myString = fm.elidedText(myString,Qt::ElideMiddle,option.rect.width()-extraSpace);

    painter->drawText(option.rect.bottomLeft()+QPoint(3+pixmap.width(),-8),myString);

    painter->restore();
}

QWidget *IdItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return new QLineEdit(parent);
}

void IdItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ModelNode node = m_TreeModel->nodeForIndex(index);
    QString value = node.id();

    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setText(value);
}

void IdItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    Q_UNUSED(model);
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    m_TreeModel->setId(index,lineEdit->text());
}

void IdItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setGeometry(option.rect);
}

}
