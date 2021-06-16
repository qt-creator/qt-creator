/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Event List module.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
******************************************************************************/

#pragma once

#include "eventlist.h"

#include <abstractview.h>
#include <QStandardItemModel>

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
    explicit EventListView(QObject *parent = nullptr);
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
    EventListModel *m_model;
};

} // namespace QmlDesigner.
