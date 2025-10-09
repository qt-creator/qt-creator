// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "interactiveconnectionmanager.h"
#include "nodeinstanceserverproxy.h"
#include "nodeinstancetracing.h"
#include "nodeinstanceview.h"

#include <qmldesignerplugin.h>

#include <qmldesignerbase/settings/designersettings.h>

#include <coreplugin/messagebox.h>

#include <QFileInfo>
#include <QLocalSocket>
#include <QTimer>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using NodeInstanceTracing::category;

InteractiveConnectionManager::InteractiveConnectionManager()
{
    NanotraceHR::Tracer tracer{"interactive connection manager constructor", category()};

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
    NanotraceHR::Tracer tracer{"interactive connection manager setup", category()};

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
    NanotraceHR::Tracer tracer{"interactive connection manager shutdown", category()};

    m_view = {};
    ConnectionManager::shutDown();
}

void InteractiveConnectionManager::showCannotConnectToPuppetWarningAndSwitchToEditMode(
    const QString &qmlPuppetPath)
{
    NanotraceHR::Tracer tracer{"interactive connection manager show cannot connect warning",
                               category(),
                               keyValue("qml puppet path", qmlPuppetPath)};

    QString title = tr("Cannot Connect to QML Puppet");
    QString message;

    if (!QFileInfo::exists(qmlPuppetPath))
        message = tr("The QML Puppet executable was not found.\n");
    else
        message = tr("The executable of the QML Puppet may not be responding.\n");
    message += tr("Switching to another kit with a correct QML Puppet might help.");

    Core::AsynchronousMessageBox::warning(title, message);

    QmlDesignerPlugin::instance()->switchToTextModeDeferred();
    if (m_view && m_view->isAttached())
        m_view->model()->emitDocumentMessage(title);
}

void InteractiveConnectionManager::dispatchCommand(const QVariant &command, Connection &connection)
{
    NanotraceHR::Tracer tracer{"interactive connection manager dispatch command",
                               category(),
                               keyValue("connection", connection)};

    static const int puppetAliveCommandType = QMetaType::fromName("PuppetAliveCommand").id();

    if (command.typeId() == puppetAliveCommandType) {
        puppetAlive(connection);
    } else {
        BaseConnectionManager::dispatchCommand(command, connection);
    }
}

void InteractiveConnectionManager::puppetTimeout(Connection &connection)
{
    NanotraceHR::Tracer tracer{"interactive connection manager puppet timeout",
                               category(),
                               keyValue("connection", connection)};

    if (connection.timer && connection.socket && connection.socket->waitForReadyRead(10)) {
        connection.timer->stop();
        connection.timer->start();
        return;
    }

    processFinished(connection.name + "_timeout");
}

void InteractiveConnectionManager::puppetAlive(Connection &connection)
{
    NanotraceHR::Tracer tracer{"interactive connection manager puppet alive",
                               category(),
                               keyValue("connection", connection)};

    if (connection.timer) {
        connection.timer->stop();
        connection.timer->start();
    }
}

} // namespace QmlDesigner
