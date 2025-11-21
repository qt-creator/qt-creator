// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quickeventreplayclient.h"

#include "qmldebugconnection.h"
#include "qmlprofilereventtypes.h"
#include "qpacketprotocol.h"

#include <QFile>
#include <QXmlStreamReader>

using namespace Qt::StringLiterals;

namespace QmlDebug {

QuickEventReplayClient::QuickEventReplayClient(QmlDebug::QmlDebugConnection *connection)
    : QmlDebugClient("EventReplay"_L1, connection)
{
}

void QuickEventReplayClient::sendEvent(const QmlEventType &type, const QmlEvent &event)
{
    QmlDebug::QPacket stream(connection()->currentDataStreamVersion());
    stream << event.timestamp() << Message::Event
           << type.detailType() << event.number<int>(0) << event.number<int>(1)
           << event.number<int>(2);
    sendMessage(stream.data());
}

} // namespace QmlDebug
