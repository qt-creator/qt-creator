// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>
#include <utils/filepath.h>

namespace ProjectExplorer { class RunControl; }

namespace Debugger::Internal {

class DebuggerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Debugger.json")

public:
    DebuggerPlugin();
    ~DebuggerPlugin() override;

private:
    // IPlugin implementation.
    bool initialize(const QStringList &arguments, QString *errorMessage) override;
    QObject *remoteCommand(const QStringList &options,
                           const QString &workingDirectory,
                           const QStringList &arguments) override;
    ShutdownFlag aboutToShutdown() override;
    void extensionsInitialized() override;

    // Called from AppOutputPane::attachToRunControl().
    Q_SLOT void attachExternalApplication(ProjectExplorer::RunControl *rc);

    // Called from GammaRayIntegration
    Q_SLOT void getEnginesState(QByteArray *json) const;

    // Called from DockerDevice
    Q_SLOT void autoDetectDebuggersForDevice(const Utils::FilePaths &searchPaths,
                                             const QString &detectionId,
                                             QString *logMessage);
    Q_SLOT void removeDetectedDebuggers(const QString &detectionId, QString *logMessage);
    Q_SLOT void listDetectedDebuggers(const QString &detectionId, QString *logMessage);

    Q_SLOT void attachToProcess(const qint64 processId, const Utils::FilePath &executable);
};

} // Debugger::Internal

Q_DECLARE_METATYPE(QString *)
