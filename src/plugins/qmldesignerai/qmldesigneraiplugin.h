// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace QmlDesigner {

class QmlDesignerAiPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlDesignerAi.json")

public:
    QmlDesignerAiPlugin();
    ~QmlDesignerAiPlugin();

private:
    virtual Utils::Result<> initialize(const QStringList &arguments) override;
    bool delayedInitialize() override;
};

} // namespace QmlDesigner
