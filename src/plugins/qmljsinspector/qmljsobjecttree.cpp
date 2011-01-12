/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "qmljsobjecttree.h"

#include <QContextMenuEvent>
#include <QEvent>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QApplication>
#include <QInputDialog>

#include <QDebug>

namespace QmlJSInspector {
namespace Internal {

// *************************************************************************
//  ObjectTreeItem
// *************************************************************************

class ObjectTreeItem : public QTreeWidgetItem
{
public:
    explicit ObjectTreeItem(QTreeWidget *widget, int type = 0);
    ObjectTreeItem(QTreeWidgetItem *parentItem, int type = 0);
    QVariant data (int column, int role) const;
    void setData (int column, int role, const QVariant & value);

    void setHasValidDebugId(bool value);


private:
    bool m_hasValidDebugId;
};

ObjectTreeItem::ObjectTreeItem(QTreeWidget *widget, int type) :
    QTreeWidgetItem(widget, type), m_hasValidDebugId(true)
{

}

ObjectTreeItem::ObjectTreeItem(QTreeWidgetItem *parentItem, int type) :
    QTreeWidgetItem(parentItem, type), m_hasValidDebugId(true)
{

}

QVariant ObjectTreeItem::data (int column, int role) const
{
    if (role == Qt::ForegroundRole)
        return m_hasValidDebugId ? qApp->palette().color(QPalette::Foreground) : qApp->palette().color(QPalette::Disabled, QPalette::Foreground);

    return QTreeWidgetItem::data(column, role);
}

void ObjectTreeItem::setData (int column, int role, const QVariant & value)
{
    QTreeWidgetItem::setData(column, role, value);
}

void ObjectTreeItem::setHasValidDebugId(bool value)
{
    m_hasValidDebugId = value;
}

// *************************************************************************
//  QmlJSObjectTree
// *************************************************************************

QmlJSObjectTree::QmlJSObjectTree(QWidget *parent)
    : QTreeWidget(parent)
    , m_clickedItem(0)
    , m_currentObjectDebugId(0)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);
    setHeaderHidden(true);
    setExpandsOnDoubleClick(false);

    m_goToFileAction = new QAction(tr("Go to file"), this);
    connect(m_goToFileAction, SIGNAL(triggered()), SLOT(goToFile()));

    connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            SLOT(currentItemChanged(QTreeWidgetItem *)));
    connect(this, SIGNAL(itemActivated(QTreeWidgetItem *, int)),
            SLOT(activated(QTreeWidgetItem *)));
    connect(this, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
}

void QmlJSObjectTree::selectionChanged()
{
    if (selectedItems().isEmpty())
        return;

    // TODO
}

void QmlJSObjectTree::setCurrentObject(int debugId)
{
    QTreeWidgetItem *item = findItemByObjectId(debugId);
    if (item) {
        setCurrentItem(item);
        scrollToItem(item);
        item->setExpanded(true);
    }
}

void QmlJSObjectTree::currentItemChanged(QTreeWidgetItem *item)
{
    if (!item)
        return;

    QDeclarativeDebugObjectReference obj = item->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>();
    if (obj.debugId() >= 0)
        emit currentObjectChanged(obj);
}

void QmlJSObjectTree::activated(QTreeWidgetItem *item)
{
    if (!item)
        return;

    QDeclarativeDebugObjectReference obj = item->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>();
    if (obj.debugId() >= 0)
        emit activated(obj);
}

void QmlJSObjectTree::buildTree(const QDeclarativeDebugObjectReference &obj, QTreeWidgetItem *parent)
{
    if (!parent)
        clear();

    if (obj.contextDebugId() < 0)
        return;

    ObjectTreeItem *item = parent ? new ObjectTreeItem(parent) : new ObjectTreeItem(this);
    if (obj.idString().isEmpty())
        item->setText(0, QString("<%1>").arg(obj.className()));
    else
        item->setText(0, obj.idString());
    item->setData(0, Qt::UserRole, qVariantFromValue(obj));

    if (parent && obj.contextDebugId() >= 0
            && obj.contextDebugId() != parent->data(0, Qt::UserRole
                    ).value<QDeclarativeDebugObjectReference>().contextDebugId())
    {

        QDeclarativeDebugFileReference source = obj.source();
        if (!source.url().isEmpty()) {
            QString toolTipString = tr("Url: ") + source.url().toString();
            item->setToolTip(0, toolTipString);
        }
    } else {
        item->setExpanded(true);
    }

    if (obj.contextDebugId() < 0)
        item->setHasValidDebugId(false);

    for (int ii = 0; ii < obj.children().count(); ++ii)
        buildTree(obj.children().at(ii), item);
}

QTreeWidgetItem *QmlJSObjectTree::findItemByObjectId(int debugId) const
{
    for (int i=0; i<topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = findItem(topLevelItem(i), debugId);
        if (item)
            return item;
    }

    return 0;
}

QTreeWidgetItem *QmlJSObjectTree::findItem(QTreeWidgetItem *item, int debugId) const
{
    if (item->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>().debugId() == debugId)
        return item;

    QTreeWidgetItem *child;
    for (int i=0; i<item->childCount(); ++i) {
        child = findItem(item->child(i), debugId);
        if (child)
            return child;
    }

    return 0;
}

void QmlJSObjectTree::goToFile()
{
    QDeclarativeDebugObjectReference obj =
            currentItem()->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>();

    if (obj.debugId() >= 0)
        emit activated(obj);
}

void QmlJSObjectTree::contextMenuEvent(QContextMenuEvent *event)
{
    m_clickedItem = itemAt(QPoint(event->pos().x(),
                                  event->pos().y() ));
    if (!m_clickedItem)
        return;

    QMenu menu;
    menu.addAction(m_goToFileAction);
    menu.exec(event->globalPos());
}

} // Internal
} // QmlJSInspector
