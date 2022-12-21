// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "modelnode.h"

#include <utils/filepath.h>

#include <QDialog>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QStandardItemModel)

namespace QmlDesigner {

class Model;
class NodeListView;
class EventListView;
class EventListPluginView;

class EventList
{
public:
    static int currentNode();
    static bool hasEventListModel();
    static bool connectedToCurrent(const QString &eventId);
    static void addEventIdToCurrent(const QString &eventId);
    static void removeEventIdFromCurrent(const QString &eventId);
    static void setNodeProperties(AbstractView *view);
    static void selectNode(int internalId);

    static QString setNodeId(int internalId, const QString &id);
    static QStandardItemModel *nodeModel();
    static NodeListView *nodeListView();
    static ModelNode modelNode(const QString &id);

    static void setSignalSource(SignalHandlerProperty &prop, const QString &source);

    EventList();
    ~EventList();

    Model *model() const;

    EventListView *view() const;
    QString read() const;

    void initialize(EventListPluginView *parent);
    void write(const QString &text);

private:
    ModelPointer m_model;
    std::unique_ptr<EventListView> m_eventView;
    Utils::FilePath m_path;
};

} // namespace QmlDesigner.
