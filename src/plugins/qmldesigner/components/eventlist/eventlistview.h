// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "eventlist.h"

#include <abstractview.h>
#include <QStandardItemModel>

#include <memory>

namespace QmlDesigner {

struct Event
{
    QString eventId;
    QString shortcut;
    QString description;
};

class EventListModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum Columns {
        idColumn = 0,
        descriptionColumn = 1,
        shortcutColumn = 2,
        connectColumn = 3
    };

    enum Roles : unsigned int {
        connectedRole = Qt::UserRole + 1,
    };

    EventListModel(QObject *parent = nullptr);

    QStringList connectedEvents() const;

    QStringList connectEvents(const QStringList &eventIds);
};


class EventListDialog;
class AssignEventDialog;
class ConnectSignalDialog;

class EventListView : public AbstractView
{
    Q_OBJECT

public:
    explicit EventListView(ExternalDependenciesInterface &externalDependencies);
    ~EventListView() override;

    void nodeRemoved(const ModelNode &removedNode,
                     const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;

    void nodeReparented(const ModelNode &node,
                        const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        PropertyChangeFlags propertyChange) override;

    EventListModel *eventListModel() const;

    void addEvent(const Event &event);
    void removeEvent(const QString &name);
    void renameEvent(const QString &oldId, const QString &newId);
    void setShortcut(const QString &id, const QString &text);
    void setDescription(const QString &id, const QString &text);

private:
    void reset();

    EventList m_eventlist;
    std::unique_ptr<EventListModel> m_model;
};

} // namespace QmlDesigner.
