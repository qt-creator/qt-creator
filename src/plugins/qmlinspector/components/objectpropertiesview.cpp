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
#include "objectpropertiesview.h"
#include "inspectortreeitems.h"
#include "inspectorcontext.h"
#include "watchtable.h"
#include "qmlinspector.h"
#include "propertytypefinder.h"

#include <extensionsystem/pluginmanager.h>
#include <qmljseditor/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsdocument.h>

#include <QtGui/QApplication>
#include <QtGui/QTreeWidget>
#include <QtGui/QLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>
#include <QtGui/QContextMenuEvent>

#include <QtCore/QFile>
#include <QtCore/QDebug>

namespace Qml {
namespace Internal {

ObjectPropertiesView::ObjectPropertiesView(WatchTableModel *watchTableModel,
                                           QDeclarativeEngineDebug *client, QWidget *parent)
    : QWidget(parent),
      m_client(client),
      m_query(0),
      m_watch(0), m_clickedItem(0),
      m_groupByItemType(true), m_showUnwatchableProperties(false),
      m_watchTableModel(watchTableModel)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    m_tree = new QTreeWidget(this);
    m_tree->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_tree->setFrameStyle(QFrame::NoFrame);
    m_tree->setAlternatingRowColors(true);
    m_tree->setExpandsOnDoubleClick(false);
    m_tree->setRootIsDecorated(m_groupByItemType);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_tree->setHeaderLabels(QStringList()
            << tr("Name") << tr("Value") << tr("Type"));
    QObject::connect(m_tree, SIGNAL(itemActivated(QTreeWidgetItem *, int)),
                     this, SLOT(itemDoubleClicked(QTreeWidgetItem *, int)));
    connect(m_tree, SIGNAL(itemSelectionChanged()), SLOT(changeItemSelection()));

    m_addWatchAction = new QAction(tr("&Watch expression"), this);
    m_removeWatchAction = new QAction(tr("&Remove watch"), this);
    m_toggleUnwatchablePropertiesAction = new QAction(tr("Show &unwatchable properties"), this);


    m_toggleGroupByItemTypeAction = new QAction(tr("&Group by item type"), this);
    m_toggleGroupByItemTypeAction->setCheckable(true);
    m_toggleGroupByItemTypeAction->setChecked(m_groupByItemType);
    connect(m_addWatchAction, SIGNAL(triggered()), SLOT(addWatch()));
    connect(m_removeWatchAction, SIGNAL(triggered()), SLOT(removeWatch()));
    connect(m_toggleUnwatchablePropertiesAction, SIGNAL(triggered()), SLOT(toggleUnwatchableProperties()));
    connect(m_toggleGroupByItemTypeAction, SIGNAL(triggered()), SLOT(toggleGroupingByItemType()));

    m_tree->setColumnCount(3);
    m_tree->header()->setDefaultSectionSize(150);

    layout->addWidget(m_tree);
}

void ObjectPropertiesView::readSettings(const InspectorSettings &settings)
{
    if (settings.showUnwatchableProperties() != m_showUnwatchableProperties)
        toggleUnwatchableProperties();
    if (settings.groupPropertiesByItemType() != m_groupByItemType)
        toggleGroupingByItemType();
}

void ObjectPropertiesView::saveSettings(InspectorSettings &settings)
{
    settings.setShowUnwatchableProperties(m_showUnwatchableProperties);
    settings.setGroupPropertiesByItemType(m_groupByItemType);
}

void ObjectPropertiesView::toggleUnwatchableProperties()
{
    m_showUnwatchableProperties = !m_showUnwatchableProperties;
    setObject(m_object);
}

void ObjectPropertiesView::toggleGroupingByItemType()
{
    m_groupByItemType = !m_groupByItemType;
    m_tree->setRootIsDecorated(m_groupByItemType);
    m_toggleGroupByItemTypeAction->setChecked(m_groupByItemType);
    setObject(m_object);
}

void ObjectPropertiesView::changeItemSelection()
{
    if (m_tree->selectedItems().isEmpty())
        return;

    QString item = m_object.className();
    QString prop = m_tree->selectedItems().first()->text(0);
    emit contextHelpIdChanged(InspectorContext::contextHelpIdForProperty(item, prop));
}

void ObjectPropertiesView::setEngineDebug(QDeclarativeEngineDebug *client)
{
    m_client = client;
}

void ObjectPropertiesView::clear()
{
    setObject(QDeclarativeDebugObjectReference());
}

void ObjectPropertiesView::reload(const QDeclarativeDebugObjectReference &obj)
{
    if (!m_client)
        return;
    if (m_query)
        delete m_query;

    m_query = m_client->queryObjectRecursive(obj, this);
    if (!m_query->isWaiting())
        queryFinished();
    else
        QObject::connect(m_query, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(queryFinished()));
}

void ObjectPropertiesView::queryFinished()
{
    if (!m_client || !m_query)
        return;

    QDeclarativeDebugObjectReference obj = m_query->object();

    QDeclarativeDebugWatch *watch = m_client->addWatch(obj, this);
    if (watch->state() == QDeclarativeDebugWatch::Dead) {
        delete watch;
        watch = 0;
    } else {
        if (m_watch) {
            m_client->removeWatch(m_watch);
            delete m_watch;
        }
        m_watch = watch;
        QObject::connect(watch, SIGNAL(valueChanged(QByteArray,QVariant)),
                        this, SLOT(valueChanged(QByteArray,QVariant)));
    }

    delete m_query;
    m_query = 0;

    setObject(obj);
}

void ObjectPropertiesView::setPropertyValue(PropertiesViewItem *item, const QVariant &value, bool makeGray)
{
    if (value.type() == QVariant::List || value.type() == QVariant::StringList) {
        PropertiesViewItem *bindingItem = static_cast<PropertiesViewItem*>(item->takeChild(item->childCount() - 1));
        if (bindingItem && bindingItem->type != PropertiesViewItem::BindingType) {
            delete bindingItem;
            bindingItem = 0;
        }

        qDeleteAll(item->takeChildren());

        QVariantList variants = value.toList();
        item->setText(1, tr("<%n items>", 0, variants.count()));
        item->setText(2, QString::fromUtf8(value.typeName()));

        PropertiesViewItem *child;
        for (int i=0; i<variants.count(); ++i) {
            child = new PropertiesViewItem(item);
            setPropertyValue(child, variants[i], makeGray);
        }

        if (bindingItem)
            item->addChild(bindingItem);

        item->setExpanded(false);
    } else {
        item->setText(1, (value.isNull() ? QLatin1String("<no value>") : value.toString()));
        item->setExpanded(true);
    }

    if (makeGray) {
        for (int i=0; i<m_tree->columnCount(); ++i)
            item->setForeground(i, Qt::gray);
    }
}

QString ObjectPropertiesView::propertyBaseClass(const QDeclarativeDebugObjectReference &object, const QDeclarativeDebugPropertyReference &property, int &depth)
{
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    QmlJSEditor::ModelManagerInterface *modelManager = pluginManager->getObject<QmlJSEditor::ModelManagerInterface>();
    QmlJS::Snapshot snapshot = modelManager->snapshot();

    QmlJS::Document::Ptr document = snapshot.document(object.source().url().path());
    if (document.isNull()) {

        QFile inFile(object.source().url().path());
        QString contents;
        if (inFile.open(QIODevice::ReadOnly)) {
            QTextStream ins(&inFile);
            contents = ins.readAll();
            inFile.close();
        }

        document = QmlJS::Document::create(object.source().url().path());
        document->setSource(contents);
        if (!document->parse())
            return QString();

        snapshot.insert(document);
    }

    PropertyTypeFinder find(document, snapshot, modelManager->importPaths());
    QString baseClassName = find(object.source().lineNumber(), object.source().columnNumber(), property.name());

    if (baseClassName.isEmpty()) {
        if (!object.idString().isEmpty())
            baseClassName = object.idString();
        else
            baseClassName = QString("<%1>").arg(object.className());
    }

    depth = find.depth();

    return baseClassName;

}

void ObjectPropertiesView::setObject(const QDeclarativeDebugObjectReference &object)
{
    m_object = object;
    m_tree->clear();

    QHash<QString, PropertiesViewItem*> baseClassItems;
    PropertiesViewItem* currentParentItem = 0;

    QList<QString> insertedPropertyNames;

    QList<QDeclarativeDebugPropertyReference> properties = object.properties();
    for (int i=0; i<properties.count(); ++i) {
        const QDeclarativeDebugPropertyReference &p = properties[i];

        // ignore overridden/redefined/shadowed properties; and do special ignore for QGraphicsObject* parent,
        // which is useless while debugging.
        if (insertedPropertyNames.contains(p.name())
            || (p.name() == "parent" && p.valueTypeName() == "QGraphicsObject*"))
        {
            continue;
        }
        insertedPropertyNames.append(p.name());

        if (m_showUnwatchableProperties || p.hasNotifySignal()) {

            PropertiesViewItem *item = 0;

            if (m_groupByItemType) {
                int depth = 0;
                QString baseClassName = propertyBaseClass(object, p, depth);
                if (!baseClassItems.contains(baseClassName)) {
                    PropertiesViewItem *baseClassItem = new PropertiesViewItem(m_tree, PropertiesViewItem::ClassType);
                    baseClassItem->setData(0, PropertiesViewItem::CanEditRole, false);
                    baseClassItem->setData(0, PropertiesViewItem::ClassDepthRole, depth);
                    baseClassItem->setText(0, baseClassName);

                    QFont font = m_tree->font();
                    font.setBold(true);
                    baseClassItem->setFont(0, font);

                    baseClassItems.insert(baseClassName, baseClassItem);

                }
                currentParentItem = baseClassItems.value(baseClassName);
                item = new PropertiesViewItem(currentParentItem);
            } else
                item = new PropertiesViewItem(m_tree);

            item->property = p;
            item->setData(0, PropertiesViewItem::ObjectIdStringRole, object.idString());
            item->setText(0, p.name());
            Qt::ItemFlags itemFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

            bool canEdit = object.idString().length() && QmlInspector::instance()->canEditProperty(item->property.valueTypeName());
            item->setData(0, PropertiesViewItem::CanEditRole, canEdit);

            if (canEdit)
                itemFlags |= Qt::ItemIsEditable;

            item->setFlags(itemFlags);
            if (m_watchTableModel.data() && m_watchTableModel.data()->isWatchingProperty(p)) {
                QFont font = m_tree->font();
                font.setBold(true);
                item->setFont(0, font);
            }

            setPropertyValue(item, p.value(), !p.hasNotifySignal());

            item->setText(2, p.valueTypeName());

            // binding is set after property value to ensure it is added to the end of the
            // list, if the value is a list
            if (!p.binding().isEmpty()) {
                PropertiesViewItem *binding = new PropertiesViewItem(item, PropertiesViewItem::BindingType);
                binding->setText(1, p.binding());
                binding->setForeground(1, Qt::darkGreen);
            }

        }
    }
    if (m_groupByItemType)
        sortBaseClassItems();
    m_tree->expandAll();

}

static bool baseClassItemsDepthLessThan(const PropertiesViewItem *item1, const PropertiesViewItem *item2)
{
    int depth1 = item1->data(0, PropertiesViewItem::ClassDepthRole).toInt();
    int depth2 = item2->data(0, PropertiesViewItem::ClassDepthRole).toInt();

    // reversed order
    return !(depth1 < depth2);
}

void ObjectPropertiesView::sortBaseClassItems()
{
    QList<PropertiesViewItem*> topLevelItems;
    while(m_tree->topLevelItemCount())
        topLevelItems.append(static_cast<PropertiesViewItem*>(m_tree->takeTopLevelItem(0)));

    qSort(topLevelItems.begin(), topLevelItems.end(), baseClassItemsDepthLessThan);
    foreach(PropertiesViewItem *item, topLevelItems)
        m_tree->addTopLevelItem(item);

}

void ObjectPropertiesView::watchCreated(QDeclarativeDebugWatch *watch)
{

    if (watch->objectDebugId() == m_object.debugId()
            && qobject_cast<QDeclarativeDebugPropertyWatch*>(watch)) {
        connect(watch, SIGNAL(stateChanged(QDeclarativeDebugWatch::State)), SLOT(watchStateChanged()));
        setWatched(qobject_cast<QDeclarativeDebugPropertyWatch*>(watch)->name(), true);
    }
}

void ObjectPropertiesView::watchStateChanged()
{
    QDeclarativeDebugWatch *watch = qobject_cast<QDeclarativeDebugWatch*>(sender());

    if (watch->objectDebugId() == m_object.debugId()
            && qobject_cast<QDeclarativeDebugPropertyWatch*>(watch)
            && watch->state() == QDeclarativeDebugWatch::Inactive) {
        setWatched(qobject_cast<QDeclarativeDebugPropertyWatch*>(watch)->name(), false);
    }
}

void ObjectPropertiesView::setWatched(const QString &property, bool watched)
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        PropertiesViewItem *topLevelItem = static_cast<PropertiesViewItem *>(m_tree->topLevelItem(i));
        if (m_groupByItemType) {
            for(int j = 0; j < topLevelItem->childCount(); ++j) {
                PropertiesViewItem *item = static_cast<PropertiesViewItem*>(topLevelItem->child(j));
                if (styleWatchedItem(item, property, watched))
                    return;
            }
        } else {
            if (styleWatchedItem(topLevelItem, property, watched))
                return;
        }
    }
}

bool ObjectPropertiesView::styleWatchedItem(PropertiesViewItem *item, const QString &property, bool isWatched) const
{
    if (item->property.name() == property && item->property.hasNotifySignal()) {
        QFont font = m_tree->font();
        font.setBold(isWatched);
        item->setFont(0, font);
        return true;
    }
    return false;
}

bool ObjectPropertiesView::isWatched(QTreeWidgetItem *item)
{
    return item->font(0).bold();
}

void ObjectPropertiesView::valueChanged(const QByteArray &name, const QVariant &value)
{
    for (int i=0; i<m_tree->topLevelItemCount(); ++i) {
        PropertiesViewItem *topLevelItem = static_cast<PropertiesViewItem *>(m_tree->topLevelItem(i));

        if (m_groupByItemType) {
            for(int j = 0; j < topLevelItem->childCount(); ++j) {
                PropertiesViewItem *item = static_cast<PropertiesViewItem*>(topLevelItem->child(j));
                if (item->property.name() == name) {
                    setPropertyValue(item, value, !item->property.hasNotifySignal());
                    return;
                }
            }
        } else {
            if (topLevelItem->property.name() == name) {
                setPropertyValue(topLevelItem, value, !topLevelItem->property.hasNotifySignal());
                return;
            }
        }

    }
}

void ObjectPropertiesView::itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (item->type() == PropertiesViewItem::ClassType)
        return;

    if (column == 0)
        toggleWatch(item);
    else if (column == 1)
        m_tree->editItem(item, column);
}

void ObjectPropertiesView::addWatch()
{
    toggleWatch(m_clickedItem);
}

void ObjectPropertiesView::removeWatch()
{
    toggleWatch(m_clickedItem);
}

void ObjectPropertiesView::toggleWatch(QTreeWidgetItem *item)
{
    if (!item)
        return;

    PropertiesViewItem *propItem = static_cast<PropertiesViewItem *>(item);
    if (!propItem->property.name().isEmpty())
        emit watchToggleRequested(m_object, propItem->property);
}

void ObjectPropertiesView::contextMenuEvent(QContextMenuEvent *event)
{
    m_clickedItem = m_tree->itemAt(QPoint(event->pos().x(),
                                          event->pos().y() - m_tree->header()->height()));
    if (!m_clickedItem)
        return;

    PropertiesViewItem *propItem = static_cast<PropertiesViewItem *>(m_clickedItem);

    QMenu menu;
    if (!isWatched(m_clickedItem)) {
        m_addWatchAction->setText(tr("Watch expression '%1'").arg(propItem->property.name()));
        menu.addAction(m_addWatchAction);
    } else {
        menu.addAction(m_removeWatchAction);
    }
    menu.addSeparator();

    if (m_showUnwatchableProperties)
        m_toggleUnwatchablePropertiesAction->setText(tr("Hide unwatchable properties"));
    else
        m_toggleUnwatchablePropertiesAction->setText(tr("Show unwatchable properties"));

    menu.addAction(m_toggleGroupByItemTypeAction);
    menu.addAction(m_toggleUnwatchablePropertiesAction);

    menu.exec(event->globalPos());
}

} // Internal
} // Qml
