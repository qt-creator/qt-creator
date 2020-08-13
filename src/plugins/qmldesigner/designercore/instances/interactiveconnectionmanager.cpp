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

#include "interactiveconnectionmanager.h"
#include "nodeinstanceserverproxy.h"
#include "nodeinstanceview.h"

#include <qmldesignerplugin.h>

#include <coreplugin/messagebox.h>

#include <QLocalSocket>
#include <QTimer>

namespace QmlDesigner {

InteractiveConnectionManager::InteractiveConnectionManager()
{
    m_connections.emplace_back("Editor", "editormode");
    m_connections.emplace_back("Render", "rendermode");
    m_connections.emplace_back("Preview", "previewmode");
}

void InteractiveConnectionManager::setUp(NodeInstanceServerProxy *nodeInstanceServerProxy,
                                         const QString &qrcMappingString,
                                         ProjectExplorer::Target *target)
{
    ConnectionManager::setUp(nodeInstanceServerProxy, qrcMappingString, target);

    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
    int timeOutTime = settings.value(DesignerSettingsKey::PUPPET_KILL_TIMEOUT).toInt();
    for (Connection &connection : m_connections)
        connection.timer->setInterval(timeOutTime);

    if (QmlDesignerPlugin::instance()
            ->settings()
            .value(DesignerSettingsKey::DEBUG_PUPPET)
            .toString()
            .isEmpty()) {
        for (Connection &connection : m_connections) {
            QObject::connect(connection.timer.get(), &QTimer::timeout, [&]() {
                puppetTimeout(connection);
            });
        }
    }
}

void InteractiveConnectionManager::showCannotConnectToPuppetWarningAndSwitchToEditMode()
{
    Core::AsynchronousMessageBox::warning(
        tr("Cannot Connect to QML Emulation Layer (QML Puppet)"),
        tr("The executable of the QML emulation layer (QML Puppet) may not be responding. "
           "Switching to another kit might help."));

    QmlDesignerPlugin::instance()->switchToTextModeDeferred();
    nodeInstanceServerProxy()->nodeInstanceView()->emitDocumentMessage(
        tr("Cannot Connect to QML Emulation Layer (QML Puppet)"));
}

void InteractiveConnectionManager::dispatchCommand(const QVariant &command, Connection &connection)
{
    static const int puppetAliveCommandType = QMetaType::type("PuppetAliveCommand");

    if (command.userType() == puppetAliveCommandType) {
        puppetAlive(connection);
    } else {
        BaseConnectionManager::dispatchCommand(command, connection);
    }
}

void InteractiveConnectionManager::puppetTimeout(Connection &connection)
{
    if (connection.socket && connection.socket->waitForReadyRead(10)) {
        connection.timer->stop();
        connection.timer->start();
        return;
    }

    processFinished();
}

void InteractiveConnectionManager::puppetAlive(Connection &connection)
{
    connection.timer->stop();
    connection.timer->start();
}

} // namespace QmlDesigner
