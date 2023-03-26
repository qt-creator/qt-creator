// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <utils/processinterface.h>

namespace RemoteLinux {

class LinuxDevice;
class SshProcessInterfacePrivate;

class REMOTELINUX_EXPORT SshProcessInterface : public Utils::ProcessInterface
{
public:
    SshProcessInterface(const LinuxDevice *linuxDevice);
    ~SshProcessInterface();

protected:
    void emitStarted(qint64 processId);
    // To be called from leaf destructor.
    // Can't call it from SshProcessInterface destructor as it calls virtual method.
    void killIfRunning();
    qint64 processId() const;
    bool runInShell(const Utils::CommandLine &command, const QByteArray &data = {});

private:
    virtual void handleStarted(qint64 processId);
    virtual void handleDone(const Utils::ProcessResultData &resultData);
    virtual void handleReadyReadStandardOutput(const QByteArray &outputData);
    virtual void handleReadyReadStandardError(const QByteArray &errorData);
    virtual void handleSendControlSignal(Utils::ControlSignal controlSignal) = 0;

    virtual QString fullCommandLine(const Utils::CommandLine &commandLine) const = 0;

    void start() final;
    qint64 write(const QByteArray &data) final;
    void sendControlSignal(Utils::ControlSignal controlSignal) final;

    friend class SshProcessInterfacePrivate;
    SshProcessInterfacePrivate *d = nullptr;
};

} // namespace RemoteLinux
