// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "connectionmanagerinterface.h"

#include <QLocalSocket>
#include <QLocalServer>
#include <QTimer>

namespace QmlDesigner {

ConnectionManagerInterface::~ConnectionManagerInterface() = default;

ConnectionManagerInterface::Connection::~Connection() = default;

ConnectionManagerInterface::Connection::Connection(const QString &name, const QString &mode)
    : name{name}
    , mode{mode}
{}

ConnectionManagerInterface::Connection::Connection(Connection &&connection) = default;

void ConnectionManagerInterface::Connection::clear()
{
    qmlPuppetProcess.reset();
    socket.reset();
    localServer.reset();
    blockSize = 0;
    lastReadCommandCounter = 0;
    timer.reset();
}

} // namespace QmlDesigner
