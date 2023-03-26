// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "eventlist.h"
#include <abstractview.h>

namespace QmlDesigner {

class EventListDialog;
class AssignEventDialog;
class ConnectSignalDialog;

class EventListPluginView : public AbstractView
{
    Q_OBJECT

public:
    EventListPluginView(ExternalDependenciesInterface &externalDepoendencies);
    ~EventListPluginView() override = default;

    void registerActions();

private:
    EventList m_eventlist;
    EventListDialog *m_eventListDialog = nullptr;
    AssignEventDialog *m_assigner = nullptr;
    ConnectSignalDialog *m_signalConnector = nullptr;
};

} // namespace QmlDesigner
