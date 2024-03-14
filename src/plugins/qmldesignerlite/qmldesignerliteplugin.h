// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace QmlDesigner {

class QmlDesignerLitePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlDesignerLite.json")

public:
    QmlDesignerLitePlugin();
    ~QmlDesignerLitePlugin();

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) override;
};

} // namespace QmlDesigner
