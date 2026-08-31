// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>

#include <QObject>
#include <QStringList>

namespace ProjectExplorer { class SshParameters; }
namespace Utils { class ProcessResultData; }

namespace Remote::Internal {

class SshConnectionHandle : public QObject
{
    Q_OBJECT

public:
    SshConnectionHandle(const ProjectExplorer::DeviceConstRef &device) : m_device(device) {}
    ~SshConnectionHandle() override { emit detachFromSharedConnection(); }

signals:
    void connected(const QString &socketFilePath);
    void disconnected(const Utils::ProcessResultData &result);

    void detachFromSharedConnection();

private:
    ProjectExplorer::DeviceConstRef m_device;
};

// Creates the pool of shared connections, which needs the main thread.
void setupSshConnectionSharing();

// Attaches the handle to the connection shared for these parameters and starts that connection
// if it is not up yet. connected() carries the control socket to route a command over,
// disconnected() the end of the connection. The handle detaches when it is destroyed. Whether
// sharing is wanted at all is the caller's decision, as it also decides whether to wait.
void attachToSharedConnection(SshConnectionHandle *handle,
                              const ProjectExplorer::SshParameters &parameters);

// The options that route a single command over the shared connection, for a caller with nothing
// to wait on. Empty while no connection is up: the command opens its own, and the one after it
// finds the shared connection.
QStringList sharedConnectionOptions(const ProjectExplorer::SshParameters &parameters);

// Ends the connections to the host instead of leaving them for the next user.
void closeSharedConnections(const ProjectExplorer::SshParameters &parameters);

} // namespace Remote::Internal
