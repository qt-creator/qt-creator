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
#include <QtGui/qevent.h>
#include <QtGui/qmenu.h>
#include <QtGui/qaction.h>

#include <QInputDialog>

#include <QDebug>

#include <private/qdeclarativedebug_p.h>

#include "objecttree.h"
#include "inspectortreeitems.h"
#include "inspectorcontext.h"

namespace Qml {
namespace Internal {

ObjectTree::ObjectTree(QDeclarativeEngineDebug *client, QWidget *parent)
    : QTreeWidget(parent),
      m_client(client),
      m_query(0), m_clickedItem(0), m_showUninspectableItems(false),
      m_currentObjectDebugId(0), m_showUninspectableOnInitDone(false)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);
    setHeaderHidden(true);
    setExpandsOnDoubleClick(false);

    m_addWatchAction = new QAction(tr("Add watch expression..."), this);
    m_toggleUninspectableItemsAction = new QAction(tr("Show uninspectable items"), this);
    m_toggleUninspectableItemsAction->setCheckable(true);
    m_goToFileAction = new QAction(tr("Go to file"), this);
    connect(m_toggleUninspectableItemsAction, SIGNAL(triggered()), SLOT(toggleUninspectableItems()));
    connect(m_addWatchAction, SIGNAL(triggered()), SLOT(addWatch()));
    connect(m_goToFileAction, SIGNAL(triggered()), SLOT(goToFile()));

    connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            SLOT(currentItemChanged(QTreeWidgetItem *)));
    connect(this, SIGNAL(itemActivated(QTreeWidgetItem *, int)),
            SLOT(activated(QTreeWidgetItem *)));
    connect(this, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
}

void ObjectTree::readSettings(const InspectorSettings &settings)
{
    if (settings.showUninspectableItems() != m_showUninspectableItems)
        toggleUninspectableItems();
}
void ObjectTree::saveSettings(InspectorSettings &settings)
{
    settings.setShowUninspectableItems(m_showUninspectableItems);
}

void ObjectTree::setEngineDebug(QDeclarativeEngineDebug *client)
{
    m_client = client;
}

void ObjectTree::toggleUninspectableItems()
{
    m_showUninspectableItems = !m_showUninspectableItems;
    m_toggleUninspectableItemsAction->setChecked(m_showUninspectableItems);
    reload(m_currentObjectDebugId);
}

void ObjectTree::selectionChanged()
{
    if (selectedItems().isEmpty())
        return;

    QTreeWidgetItem *item = selectedItems().first();
    if (item)
        emit contextHelpIdChanged(InspectorContext::contextHelpIdForItem(item->text(0)));
}

void ObjectTree::reload(int objectDebugId)
{
    if (!m_client)
        return;

    if (m_query) {
        delete m_query;
        m_query = 0;
    }
    m_currentObjectDebugId  = objectDebugId;

    m_query = m_client->queryObjectRecursive(QDeclarativeDebugObjectReference(objectDebugId), this);
    if (!m_query->isWaiting())
        objectFetched();
    else
        QObject::connect(m_query, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(objectFetched(QDeclarativeDebugQuery::State)));
}

void ObjectTree::setCurrentObject(int debugId)
{
    QTreeWidgetItem *item = findItemByObjectId(debugId);
    if (item) {
        setCurrentItem(item);
        scrollToItem(item);
        item->setExpanded(true);
    }


}

void ObjectTree::objectFetched(QDeclarativeDebugQuery::State state)
{
    if (state == QDeclarativeDebugQuery::Completed) {
        //dump(m_query->object(), 0);
        buildTree(m_query->object(), 0);
        setCurrentItem(topLevelItem(0));

        delete m_query;
        m_query = 0;

        // this ugly hack is needed if user wants to see internal structs
        // on startup - debugger does not load them until towards the end,
        // so essentially loading twice gives us the full list as everything
        // is already loaded.
        if (m_showUninspectableItems && !m_showUninspectableOnInitDone) {
            m_showUninspectableOnInitDone = true;
            reload(m_currentObjectDebugId);
        }
    }
}

void ObjectTree::currentItemChanged(QTreeWidgetItem *item)
{
    if (!item)
        return;

    QDeclarativeDebugObjectReference obj = item->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>();
    if (obj.debugId() >= 0)
        emit currentObjectChanged(obj);
}

void ObjectTree::activated(QTreeWidgetItem *item)
{
    if (!item)
        return;

    QDeclarativeDebugObjectReference obj = item->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>();
    if (obj.debugId() >= 0)
        emit activated(obj);
}

void ObjectTree::cleanup()
{
    m_showUninspectableOnInitDone = false;
    clear();
}

void ObjectTree::buildTree(const QDeclarativeDebugObjectReference &obj, QTreeWidgetItem *parent)
{
    if (!parent)
        clear();

    if (obj.contextDebugId() < 0 && !m_showUninspectableItems)
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
            QString toolTipString = QLatin1String("URL: ") + source.url().toString();
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

void ObjectTree::dump(const QDeclarativeDebugContextReference &ctxt, int ind)
{
    QByteArray indent(ind * 4, ' ');
    qWarning().nospace() << indent.constData() << ctxt.debugId() << " "
                         << qPrintable(ctxt.name());

    for (int ii = 0; ii < ctxt.contexts().count(); ++ii)
        dump(ctxt.contexts().at(ii), ind + 1);

    for (int ii = 0; ii < ctxt.objects().count(); ++ii)
        dump(ctxt.objects().at(ii), ind);
}

void ObjectTree::dump(const QDeclarativeDebugObjectReference &obj, int ind)
{
    QByteArray indent(ind * 4, ' ');
    qWarning().nospace() << indent.constData() << qPrintable(obj.className())
                         << " " << qPrintable(obj.idString()) << " "
                         << obj.debugId();

    for (int ii = 0; ii < obj.children().count(); ++ii)
        dump(obj.children().at(ii), ind + 1);
}

QTreeWidgetItem *ObjectTree::findItemByObjectId(int debugId) const
{
    for (int i=0; i<topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = findItem(topLevelItem(i), debugId);
        if (item)
            return item;
    }

    return 0;
}

QTreeWidgetItem *ObjectTree::findItem(QTreeWidgetItem *item, int debugId) const
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

void ObjectTree::addWatch()
{
    QDeclarativeDebugObjectReference obj =
            currentItem()->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>();

    bool ok = false;
    QString watch = QInputDialog::getText(this, tr("Watch expression"),
            tr("Expression:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !watch.isEmpty())
        emit expressionWatchRequested(obj, watch);

}

void ObjectTree::goToFile()
{
    QDeclarativeDebugObjectReference obj =
            currentItem()->data(0, Qt::UserRole).value<QDeclarativeDebugObjectReference>();

    if (obj.debugId() >= 0)
        emit activated(obj);
}

void ObjectTree::contextMenuEvent(QContextMenuEvent *event)
{

    m_clickedItem = itemAt(QPoint(event->pos().x(),
                                  event->pos().y() ));
    if (!m_clickedItem)
        return;

    QMenu menu;
    menu.addAction(m_addWatchAction);
    menu.addAction(m_goToFileAction);
    if (m_currentObjectDebugId) {
        menu.addSeparator();
        menu.addAction(m_toggleUninspectableItemsAction);
    }

    menu.exec(event->globalPos());
}

} // Internal
} // Qml
