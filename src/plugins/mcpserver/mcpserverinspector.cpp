// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpserverinspector.h"

#include "mcpservertr.h"

#include <coreplugin/icore.h>

#include <QWidget>

using namespace Utils;

namespace Mcp::Internal {

static JsonRpcInspector::Settings makeSettings()
{
    JsonRpcInspector::Settings s;
    s.windowTitle = Tr::tr("MCP Server Inspector");
    s.endpointLabel = Tr::tr("Session:");
    s.registerWindow = [](QWidget *widget) {
        Core::ICore::registerWindow(widget, Core::Context("McpClient.Inspector"));
        QObject::connect(
            Core::ICore::instance(), &Core::ICore::coreAboutToClose, widget, &QWidget::close);
    };
    return s;
}

McpInspector::McpInspector()
    : JsonRpcInspector(makeSettings())
{}

QString McpInspector::sessionIdToDisplayName(const QString &sessionId)
{
    return sessionId.isEmpty() ? Tr::tr("Global") : sessionId;
}

} // namespace Mcp::Internal
