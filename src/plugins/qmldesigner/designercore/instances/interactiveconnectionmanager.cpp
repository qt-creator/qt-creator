// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    connections().emplace_back("Editor", "editormode");
    connections().emplace_back("Render", "rendermode");
    connections().emplace_back("Preview", "previewmode");
}

void InteractiveConnectionManager::setUp(NodeInstanceServerInterface *nodeInstanceServer,
                                         const QString &qrcMappingString,
                                         ProjectExplorer::Target *target,
                                         AbstractView *view,
                                         ExternalDependenciesInterface &externalDependencies)
{
    ConnectionManager::setUp(nodeInstanceServer, qrcMappingString, target, view, externalDependencies);

    int timeOutTime = QmlDesignerPlugin::settings()
            .value(DesignerSettingsKey::PUPPET_KILL_TIMEOUT).toInt();
    for (Connection &connection : connections()) {
        connection.timer.reset(new QTimer);
        connection.timer->setInterval(timeOutTime);
    }

    if (QmlDesignerPlugin::settings()
            .value(DesignerSettingsKey::DEBUG_PUPPET)
            .toString()
            .isEmpty()) {
        for (Connection &connection : connections()) {
            QObject::connect(connection.timer.get(), &QTimer::timeout, [&]() {
                puppetTimeout(connection);
            });
        }
    }
}

void InteractiveConnectionManager::shutDown()
{
    m_view = {};
    ConnectionManager::shutDown();
}

void InteractiveConnectionManager::showCannotConnectToPuppetWarningAndSwitchToEditMode()
{
    Core::AsynchronousMessageBox::warning(
        tr("Cannot Connect to QML Emulation Layer (QML Puppet)"),
        tr("The executable of the QML emulation layer (QML Puppet) may not be responding. "
           "Switching to another kit might help."));

    QmlDesignerPlugin::instance()->switchToTextModeDeferred();
    if (m_view)
        m_view->emitDocumentMessage(tr("Cannot Connect to QML Emulation Layer (QML Puppet)"));
}

void InteractiveConnectionManager::dispatchCommand(const QVariant &command, Connection &connection)
{
    static const int puppetAliveCommandType = QMetaType::type("PuppetAliveCommand");

    if (command.typeId() == puppetAliveCommandType) {
        puppetAlive(connection);
    } else {
        BaseConnectionManager::dispatchCommand(command, connection);
    }
}

void InteractiveConnectionManager::puppetTimeout(Connection &connection)
{
    if (connection.timer && connection.socket && connection.socket->waitForReadyRead(10)) {
        connection.timer->stop();
        connection.timer->start();
        return;
    }

    processFinished(connection.name + "_timeout");
}

void InteractiveConnectionManager::puppetAlive(Connection &connection)
{
    if (connection.timer) {
        connection.timer->stop();
        connection.timer->start();
    }
}

} // namespace QmlDesigner
