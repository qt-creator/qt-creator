// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>
#include <projectexplorer/projectexplorer.h>

namespace Valgrind::Internal {

class ValgrindPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Valgrind.json")

public:
    ValgrindPlugin() = default;
    ~ValgrindPlugin() final;

    bool initialize(const QStringList &arguments, QString *errorString) final;

private:
    QVector<QObject *> createTestObjects() const override;

    class ValgrindPluginPrivate *d = nullptr;
};

} // Valgrind::Internal
