// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Qdb {
namespace Internal {

class QdbPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Boot2Qt.json")

public:
    QdbPlugin() = default;
    ~QdbPlugin() final;

private:
    bool initialize(const QStringList &arguments, QString *errorString) final;
    void extensionsInitialized() final;
    ShutdownFlag aboutToShutdown() final;

    class QdbPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace Qdb
