// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "deviceprocesslist.h"

#include <memory>

namespace ProjectExplorer {

class SshDeviceProcessListPrivate;

class PROJECTEXPLORER_EXPORT SshDeviceProcessList : public DeviceProcessList
{
    Q_OBJECT
public:
    explicit SshDeviceProcessList(const IDeviceConstPtr &device, QObject *parent = nullptr);
    ~SshDeviceProcessList() override;

private:
    void handleProcessDone();
    void handleKillProcessFinished(const QString &errorString);

    virtual QString listProcessesCommandLine() const = 0;
    virtual QList<Utils::ProcessInfo> buildProcessList(const QString &listProcessesReply) const = 0;

    void doUpdate() override;
    void doKillProcess(const Utils::ProcessInfo &process) override;

    void setFinished();

    const std::unique_ptr<SshDeviceProcessListPrivate> d;
};

} // namespace ProjectExplorer
