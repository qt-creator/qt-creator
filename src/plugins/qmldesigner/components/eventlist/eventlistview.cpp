/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include "eventlistview.h"

#include "assigneventdialog.h"
#include "connectsignaldialog.h"
#include "eventlistactions.h"
#include "eventlistdialog.h"
#include "nodelistproperty.h"
#include "nodeproperty.h"
#include "variantproperty.h"
#include <coreplugin/icore.h>
#include <componentcore/componentcore_constants.h>

#include <QPushButton>
#include <QStandardItemModel>

namespace QmlDesigner {

EventListModel::EventListModel(QObject *parent)
    : QStandardItemModel(0, 4, parent)
{
    setHeaderData(idColumn, Qt::Horizontal, tr("Event ID"));
    setHeaderData(shortcutColumn, Qt::Horizontal, tr("Shortcut"));
    setHeaderData(descriptionColumn, Qt::Horizontal, tr("Description"));
    setHeaderData(connectColumn, Qt::Horizontal, tr(""));
}

QStringList EventListModel::connectedEvents() const
{
    QStringList out;
    for (int row = 0; row < rowCount(); ++row) {
        if (auto idx = index(row, EventListModel::idColumn); idx.isValid()) {
            if (idx.data(EventListModel::connectedRole).toBool())
                out << idx.data().toString();
        }
    }
    return out;
}

QStringList EventListModel::connectEvents(const QStringList &eventIds)
{
    auto out = eventIds;
    for (int row = 0; row < rowCount(); ++row) {
        auto nameIndex = index(row, EventListModel::idColumn);
        bool connected = out.removeOne(nameIndex.data().toString());
        for (int col = 0; col < columnCount(); col++)
            setData(index(row, col), connected, EventListModel::connectedRole);
    }
    return out;
}


EventListView::EventListView(QObject *parent)
    : AbstractView(parent)
    , m_eventlist()
    , m_model(new EventListModel(this))
{}

EventListView::~EventListView() {}

void EventListView::nodeRemoved(const ModelNode &removedNode,
                                const NodeAbstractProperty &parentProperty,
                                PropertyChangeFlags propertyChange)
{
    AbstractView::nodeRemoved(removedNode, parentProperty, propertyChange);
    reset();
}

void EventListView::nodeReparented(const ModelNode &node,
                                   const NodeAbstractProperty &newPropertyParent,
                                   const NodeAbstractProperty &oldPropertyParent,
                                   PropertyChangeFlags propertyChange)
{
    AbstractView::nodeReparented(node, newPropertyParent, oldPropertyParent, propertyChange);
    reset();
}

EventListModel *EventListView::eventListModel() const
{
    return m_model;
}

void EventListView::addEvent(const Event &event)
{
    executeInTransaction("EventListView::addEvent", [=]() {

        QByteArray unqualifiedTypeName = "ListElement";
        auto metaInfo = model()->metaInfo(unqualifiedTypeName);

        QByteArray fullTypeName = metaInfo.typeName();
        int minorVersion = metaInfo.minorVersion();
        int majorVersion = metaInfo.majorVersion();

        ModelNode eventNode = createModelNode(fullTypeName, majorVersion, minorVersion);
        eventNode.variantProperty("eventId").setValue(event.eventId);

        if (!event.shortcut.isEmpty())
            eventNode.variantProperty("shortcut").setValue(event.shortcut);

        if (!event.description.isEmpty())
            eventNode.variantProperty("eventDescription").setValue(event.description);

        rootModelNode().defaultNodeListProperty().reparentHere(eventNode);
    });
}

void EventListView::removeEvent(const QString &eventId)
{
    executeInTransaction("EventListView::removeEvent", [=]() {
        for (auto node : rootModelNode().defaultNodeListProperty().toModelNodeList()) {
            if (node.variantProperty("eventId").value().toString() == eventId) {
                node.destroy();
                return;
            }
        }
    });
}

void EventListView::renameEvent(const QString &oldId, const QString &newId)
{
    executeInTransaction("EventListView::renameEvent", [=]() {
        for (auto node : rootModelNode().defaultNodeListProperty().toModelNodeList()) {
            if (node.variantProperty("eventId").value().toString() == oldId) {
                node.variantProperty("eventId").setValue(newId);
                return;
            }
        }
    });
}

void EventListView::setShortcut(const QString &id, const QString &text)
{
    executeInTransaction("EventListView::setShortcut", [=]() {
        for (auto node : rootModelNode().defaultNodeListProperty().toModelNodeList()) {
            if (node.variantProperty("eventId").value().toString() == id) {
                node.variantProperty("shortcut").setValue(text);
                return;
            }
        }
    });
}

void EventListView::setDescription(const QString &id, const QString &text)
{
    executeInTransaction("EventListView::setDescription", [=]() {
        for (auto node : rootModelNode().defaultNodeListProperty().toModelNodeList()) {
            if (node.variantProperty("eventId").value().toString() == id) {
                node.variantProperty("eventDescription").setValue(text);
                return;
            }
        }
    });
}

void EventListView::reset()
{
    auto setData = [this](int row, int column, const QVariant &data) {
        QModelIndex idx = m_model->index(row, column, QModelIndex());
        m_model->setData(idx, data);
    };

    if (rootModelNode().isValid()) {
        m_model->removeRows(0, m_model->rowCount(QModelIndex()), QModelIndex());
        for (auto itemNode : rootModelNode().directSubModelNodes()) {
            int row = m_model->rowCount();
            if (m_model->insertRows(row, 1, QModelIndex())) {
                QVariant eventId = itemNode.variantProperty("eventId").value();
                QVariant eventShortcut = itemNode.variantProperty("shortcut").value();
                QVariant eventDescription = itemNode.variantProperty("eventDescription").value();

                setData(row, EventListModel::idColumn, eventId);
                setData(row, EventListModel::shortcutColumn, eventShortcut);
                setData(row, EventListModel::descriptionColumn, eventDescription);
            }
        }
    }
}

} // namespace QmlDesigner.
