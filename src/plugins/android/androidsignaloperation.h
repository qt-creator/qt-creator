// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>

#include <QObject>
#include <QTimer>

namespace Android {
namespace Internal {

class AndroidSignalOperation : public ProjectExplorer::DeviceProcessSignalOperation
{
public:
    ~AndroidSignalOperation() override;
    void killProcess(qint64 pid) override;
    void killProcess(const QString &filePath) override;
    void interruptProcess(qint64 pid) override;
    void interruptProcess(const QString &filePath) override;

protected:
    explicit AndroidSignalOperation();

private:
    enum State {
        Idle,
        RunAs,
        Kill
    };

    using FinishHandler = std::function<void()>;

    bool handleCrashMessage();
    void adbFindRunAsFinished();
    void adbKillFinished();
    void handleTimeout();

    void signalOperationViaADB(qint64 pid, int signal);
    void startAdbProcess(State state, const Utils::CommandLine &commandLine, FinishHandler handler);

    Utils::FilePath m_adbPath;
    std::unique_ptr<Utils::Process> m_adbProcess;
    QTimer *m_timeout;

    State m_state = Idle;
    qint64 m_pid = 0;
    int m_signal = 0;

    friend class AndroidDevice;
};

} // namespace Internal
} // namespace Android
