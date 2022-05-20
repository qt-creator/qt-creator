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
#pragma once

#include "modelnode.h"

#include <utils/filepath.h>

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
