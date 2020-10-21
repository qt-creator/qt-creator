/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "testconnectionmanager.h"
#include "synchronizecommand.h"

#include <QLocalSocket>

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

    if (command.userType() == synchronizeCommandType) {
        SynchronizeCommand synchronizeCommand = command.value<SynchronizeCommand>();
        m_synchronizeId = synchronizeCommand.synchronizeId();
    } else {
        ConnectionManager::dispatchCommand(command, connection);
    }
}

} // namespace QmlDesigner
