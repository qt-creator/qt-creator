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

#include "modelnode.h"
#include "utils/fileutils.h"

#include <unordered_set>
#include <QDialog>

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

    Model *model() const;

    EventListView *view() const;
    QString read() const;

    void initialize(EventListPluginView *parent);
    void write(const QString &text);

private:
    static NodeListView *st_nodeView;

    Model *m_model;
    EventListView *m_eventView;
    Utils::FilePath m_path;
};

} // namespace QmlDesigner.
