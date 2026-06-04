// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpinspector.h"

#include "acpclienttr.h"

#include <coreplugin/icore.h>

#include <QWidget>

using namespace Utils;

namespace AcpClient::Internal {

static JsonRpcInspector::Settings makeSettings()
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
    return s;
}

AcpInspector::AcpInspector()
    : JsonRpcInspector(makeSettings())
{}

} // namespace AcpClient::Internal
