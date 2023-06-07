// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testconnectionmanager.h"
#include "synchronizecommand.h"

#include <QLocalSocket>
#include <QVariant>

namespace QmlDesigner {

TestConnectionManager::TestConnectionManager()
{
    connections().emplace_back("Editor", "editormode");
}

void TestConnectionManager::writeCommand(const QVariant &command)
{
    ConnectionManager::writeCommand(command);

    writeCommandCounter()++;

    static int synchronizeId = 0;
    synchronizeId++;
    SynchronizeCommand synchronizeCommand(synchronizeId);

    QLocalSocket *socket = connections().front().socket.get();

    writeCommandToIODevice(QVariant::fromValue(synchronizeCommand), socket, writeCommandCounter());
    writeCommandCounter()++;

    while (socket->waitForReadyRead(100)) {
        readDataStream(connections().front());
        if (m_synchronizeId == synchronizeId)
            return;
    }
}

void TestConnectionManager::dispatchCommand(const QVariant &command, Connection &connection)
{
    static const int synchronizeCommandType = QMetaType::type("SynchronizeCommand");

    if (command.typeId() == synchronizeCommandType) {
        SynchronizeCommand synchronizeCommand = command.value<SynchronizeCommand>();
        m_synchronizeId = synchronizeCommand.synchronizeId();
    } else {
        ConnectionManager::dispatchCommand(command, connection);
    }
}

} // namespace QmlDesigner
