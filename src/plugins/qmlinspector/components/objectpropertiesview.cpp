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
#include "inspectorcontext.h"

#include <QtCore/QDebug>

#include <QtGui/QTreeWidget>
#include <QtGui/QLayout>
#include <QtGui/QHeaderView>
namespace Qml {
namespace Internal {


class PropertiesViewItem : public QObject, public QTreeWidgetItem
{
    Q_OBJECT
public:
    enum Type {
        BindingType,
        OtherType
    };

    PropertiesViewItem(QTreeWidget *widget, Type type = OtherType);
    PropertiesViewItem(QTreeWidgetItem *parent, Type type = OtherType);

    QDeclarativeDebugPropertyReference property;
    Type type;
};

PropertiesViewItem::PropertiesViewItem(QTreeWidget *widget, Type type)
    : QTreeWidgetItem(widget), type(type)
{
}

PropertiesViewItem::PropertiesViewItem(QTreeWidgetItem *parent, Type type)
    : QTreeWidgetItem(parent), type(type)
{
}

ObjectPropertiesView::ObjectPropertiesView(QDeclarativeEngineDebug *client, QWidget *parent)
    : QWidget(parent),
      m_client(client),
      m_query(0),
      m_watch(0)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    m_tree = new QTreeWidget(this);
    m_tree->setFrameStyle(QFrame::NoFrame);
    m_tree->setAlternatingRowColors(true);
    m_tree->setExpandsOnDoubleClick(false);
    m_tree->setRootIsDecorated(false);
    m_tree->setHeaderLabels(QStringList()
            << tr("Name") << tr("Value") << tr("Type"));
    QObject::connect(m_tree, SIGNAL(itemActivated(QTreeWidgetItem *, int)),
                     this, SLOT(itemActivated(QTreeWidgetItem *)));
    connect(m_tree, SIGNAL(itemSelectionChanged()), SLOT(changeItemSelection()));

    m_tree->setColumnCount(3);
    m_tree->header()->setDefaultSectionSize(150);

    layout->addWidget(m_tree);
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

void ObjectPropertiesView::setObject(const QDeclarativeDebugObjectReference &object)
{
    m_object = object;
    m_tree->clear();

    QList<QDeclarativeDebugPropertyReference> properties = object.properties();
    for (int i=0; i<properties.count(); ++i) {
        const QDeclarativeDebugPropertyReference &p = properties[i];

        PropertiesViewItem *item = new PropertiesViewItem(m_tree);
        item->property = p;

        item->setText(0, p.name());
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

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
    for (int i=0; i<m_tree->topLevelItemCount(); ++i) {
        PropertiesViewItem *item = static_cast<PropertiesViewItem *>(m_tree->topLevelItem(i));
        if (item->property.name() == property && item->property.hasNotifySignal()) {
            QFont font = m_tree->font();
            font.setBold(watched);
            item->setFont(0, font);
        }
    }
}

void ObjectPropertiesView::valueChanged(const QByteArray &name, const QVariant &value)
{
    for (int i=0; i<m_tree->topLevelItemCount(); ++i) {
        PropertiesViewItem *item = static_cast<PropertiesViewItem *>(m_tree->topLevelItem(i));
        if (item->property.name() == name) {
            setPropertyValue(item, value, !item->property.hasNotifySignal());
            return;
        }
    }
}

void ObjectPropertiesView::itemActivated(QTreeWidgetItem *i)
{
    PropertiesViewItem *item = static_cast<PropertiesViewItem *>(i);
    if (!item->property.name().isEmpty())
        emit activated(m_object, item->property);
}

}
}

#include "objectpropertiesview.moc"
