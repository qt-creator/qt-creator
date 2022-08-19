// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "basetoolsclient.h"

namespace QmlDebug {

BaseToolsClient::BaseToolsClient(QmlDebugConnection* client, QLatin1String clientName)
    : QmlDebugClient(clientName, client)
{
    setObjectName(clientName);
}

void BaseToolsClient::stateChanged(State state)
{
    emit newState(state);
}

void BaseToolsClient::recurseObjectIdList(const ObjectReference &ref,
                         QList<int> &debugIds, QList<QString> &objectIds)
{
    debugIds << ref.debugId();
    objectIds << ref.idString();
    foreach (const ObjectReference &child, ref.children())
        recurseObjectIdList(child, debugIds, objectIds);
}

} // namespace QmlDebug
