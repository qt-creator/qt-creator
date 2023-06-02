// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include "linuxdevice.h"

#include <utils/processinterface.h>

namespace RemoteLinux {

class SshProcessInterfacePrivate;

class REMOTELINUX_EXPORT SshProcessInterface : public Utils::ProcessInterface
{
public:
    explicit SshProcessInterface(const ProjectExplorer::IDevice::ConstPtr &device);
    ~SshProcessInterface();

protected:
    void emitStarted(qint64 processId);
    void killIfRunning();
    qint64 processId() const;
    bool runInShell(const Utils::CommandLine &command, const QByteArray &data = {});

private:
    virtual void handleSendControlSignal(Utils::ControlSignal controlSignal);

    void start() final;
    qint64 write(const QByteArray &data) final;
    void sendControlSignal(Utils::ControlSignal controlSignal) final;

    friend class SshProcessInterfacePrivate;
    SshProcessInterfacePrivate *d = nullptr;
};

} // namespace RemoteLinux
