// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpinspector.h"

#include "acpclienttr.h"

#include <coreplugin/icore.h>

#include <QTreeView>
#include <QWidget>

using namespace Utils;

namespace AcpClient::Internal {

static JsonRpcInspector::Settings makeSettings(AcpInspector *self)
{
    JsonRpcInspector::Settings s;
    s.windowTitle = Tr::tr("ACP Inspector");
    s.endpointLabel = Tr::tr("ACP Client:");
    s.defaultExpandedKeys = {"params", "result", "error"};
    s.registerWindow = [](QWidget *widget) {
        Core::ICore::registerWindow(widget, Core::Context("AcpClient.Inspector"));
        QObject::connect(
            Core::ICore::instance(), &Core::ICore::coreAboutToClose, widget, &QWidget::close);
    };
    s.extraTabs = [self](const QString &endpoint) {
        QList<QPair<QWidget *, QString>> tabs;
        QTreeView *view = createJsonTreeView(Tr::tr("Agent Capabilities"),
                                             self->capabilities(endpoint));
        tabs.append({view, Tr::tr("Capabilities")});
        return tabs;
    };
    return s;
}

AcpInspector::AcpInspector()
    : JsonRpcInspector(makeSettings(this))
{}

void AcpInspector::setCapabilities(const QString &endpoint, const QJsonObject &capabilities)
{
    m_capabilities.insert(endpoint, capabilities);
    refreshEndpoint(endpoint);
}

QJsonObject AcpInspector::capabilities(const QString &endpoint) const
{
    return m_capabilities.value(endpoint);
}

} // namespace AcpClient::Internal
